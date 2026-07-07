
import os
import glob
import time
import tifffile as tiflib
import torch
import numpy as np
#import albumentations as A
#from albumentations.pytorch import ToTensorV2
import segmentation_models_pytorch as smp

from load_gis_geom import extract_3d_buildings
from load_gis_geom import extract_building_height_raster

from py_engine3d import set_geometry
from py_engine3d import should_run
from py_engine3d import should_halt
from py_engine3d import reshade
from py_engine3d import push_texture_pixels
from py_engine3d import push_tmy_data
from py_engine3d import sent_px_cmd
from py_engine3d import GeoData
from py_engine3d import push_geodata

from geotiff_metadata import get_geotiff_metadata
from load_topo_data import get_elevation_matrix
from load_topo_data import topo_geom
from polygonize_mask import triangulate_mask

import tmy_data

from pytorch_config import setup_hardware_environment
from paths_config import PATHS
from test_img_paths import GET_INRIA_TEST_IMG_PATHS

import warnings
warnings.filterwarnings("ignore")

# Params
MODEL_PATH_PTH = PATHS["WEIGHTS"]["SELECTED_WEIGHTS"]
MODEL_PATH_OV = PATHS["OPENVINO"]["MODEL_PATH_OV"]

IMG_WINDOW_SIZE = 512
THRESHOLD = 0.3

# Init hardware
ENGINE, DEVICE = setup_hardware_environment(MODEL_PATH_OV)
print(f"Active Runtime Configuration -> Engine: {ENGINE.upper()} | Target Device: {DEVICE}")

# Global model state
compiled_model = None
output_layer = None
model = None

if ENGINE == "openvino":
    import openvino as ov
    core = ov.Core()
    ov_model = core.read_model(MODEL_PATH_OV)
    compiled_model = core.compile_model(ov_model, DEVICE)
    output_layer = compiled_model.output(0)
else:
    model = smp.Unet(encoder_name="resnet34", encoder_weights=None, in_channels=3, classes=1)
    weights = torch.load(MODEL_PATH_PTH, map_location=DEVICE, weights_only=True)
    model.load_state_dict(weights)
    model.to(DEVICE)
    model.eval()

def test_transform_pytorch(patch):
    # From (H, W, C) convert to (C, H, W) Tensor
    tensor = torch.from_numpy(patch).permute(2, 0, 1).to(dtype=torch.float32) / 255.0
    
    # Normalization
    mean = torch.tensor([0.485, 0.456, 0.406]).view(3, 1, 1)
    std = torch.tensor([0.229, 0.224, 0.225]).view(3, 1, 1)
    
    return (tensor - mean) / std

def process_and_triangulate_image(image_path):
    """
    Runs inference for high-resolution satellite imagery with image windowing at 512x512 pixels
    """
    base_name = os.path.basename(image_path)
    output_dir = "output_images/bmasks"
    output_dir = os.path.realpath(output_dir)
    output_path = os.path.join(output_dir, base_name)
    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    image = tiflib.imread(image_path)
    
    # Format satellite image
    if image.dtype == object or str(image.dtype).startswith("bfloat"):
        image = image.astype(np.float32)
    if len(image.shape) == 3 and image.shape[2] > 3:
        image = image[:, :, :3]
    if len(image.shape) == 2:
        image = np.stack([image, image, image], axis=-1)
        
    if np.issubdtype(image.dtype, np.floating):
        if image.max() <= 1.01:
            image = (image * 255).astype(np.uint8)
        else:
            image = np.clip(image, 0, 255).astype(np.uint8)
    elif image.dtype == np.uint16:
        image = (image / 256).astype(np.uint8)
    else:
        image = image.astype(np.uint8)
        
    h, w, c = image.shape
    
    # Prepare buffers for the rendering pipeline
    global_vertices = []
    global_indices = []
    
    y_coords = list(range(0, h - IMG_WINDOW_SIZE, IMG_WINDOW_SIZE)) + [h - IMG_WINDOW_SIZE]
    x_coords = list(range(0, w - IMG_WINDOW_SIZE, IMG_WINDOW_SIZE)) + [w - IMG_WINDOW_SIZE]
    
    y_coords = sorted(list(set(y_coords)))
    x_coords = sorted(list(set(x_coords)))

    # Get pixel resolution for pixel=meter conversion
    
    geodata, bounds, px_res, img_dim = get_geotiff_metadata(filepath=image_path, silent=True)

    elevation_matrix, avg_height, heightmap_size, heightmap_bounds, lat_lon_bounds = get_elevation_matrix(filepath=image_path)
    elevation_data = {
        "elevation_matrix": elevation_matrix,
        "average_height": avg_height,
        "heightmap_scale": heightmap_size,
        "heightmap_bounds": heightmap_bounds
    }
    topo_geom(
        global_vertices=global_vertices, global_indices=global_indices,
        elevation_data=elevation_data, silent=True
    )

    geodata.append(float(avg_height))
    push_geodata(GeoData(*geodata))

    clat, clon = (lat_lon_bounds[0] + lat_lon_bounds[1]) / 2, (lat_lon_bounds[2] + lat_lon_bounds[3]) / 2

    tmy_tex, sun_tex = tmy_data.get_tmy_data(clat, clon)

    print("[GIS] Typical meteorological year data rechieved (TMY)")

    global_buildings_mask = np.zeros((img_dim[1], img_dim[0]), dtype=np.uint8)

    id_poly_cnt = 2
    
    if os.path.exists(output_path):
        print(f"[SEGMENTATION_CACHE] Building Mask found at {output_path}")
        global_buildings_mask = tiflib.imread(output_path)
        # Extract mask patches
        for y in y_coords:
            for x in x_coords:
                prob_patch = global_buildings_mask[y:y+IMG_WINDOW_SIZE, x:x+IMG_WINDOW_SIZE].copy()
                
                global_vertices, global_indices, id_poly_cnt = triangulate_mask(
                    prob_mask = prob_patch,
                    global_vertices = global_vertices,
                    global_indices = global_indices,
                    id_poly = id_poly_cnt,
                    offset = (x, y),
                    world_scale = px_res,
                    elevation_data = elevation_data,
                    threshold = 127,
                    tolerance = 1.0
                )
    else:
        # Inference execution
        if ENGINE == "openvino":
            i_p = 0
            for y in y_coords:
                for x in x_coords:
                    i_p += 1
                    if i_p % 10 == 0:
                        print(f"[SEGMENTATION] Progress: { ( i_p * 100 / ( len(x_coords) * len(y_coords) ) ) } %")

                    if not should_run():
                        break
                    while should_halt():
                        time.sleep(0.125)

                    patch = image[y:y+IMG_WINDOW_SIZE, x:x+IMG_WINDOW_SIZE].copy()

                    # Normalization using NumPy equivalent to albumentations
                    patch_tensor = (patch.astype(np.float32) / 255.0 - [0.485, 0.456, 0.406]) / [0.229, 0.224, 0.225]
                    input_data = np.expand_dims(np.transpose(patch_tensor, (2, 0, 1)), axis=0)
                
                    results = compiled_model([input_data])[output_layer]
                    prob_matrix = 1 / (1 + np.exp(-results.squeeze()))

                    patch_mask_uint8 = (prob_matrix > THRESHOLD).astype(np.uint8) * 255

                    p_h, p_w = patch_mask_uint8.shape
                    global_buildings_mask[y:y+p_h, x:x+p_w] = patch_mask_uint8
                
                    # Send probability matrix to triangulate
                    global_vertices, global_indices, id_poly = triangulate_mask(
                        prob_mask = prob_matrix,
                        global_vertices = global_vertices,
                        global_indices = global_indices,
                        id_poly = id_poly_cnt,
                        offset = (x,y),
                        world_scale = px_res,
                        elevation_data = elevation_data,
                        threshold = THRESHOLD,
                        tolerance = 1.0
                    )
        else:
            # PyTorch Pipeline
            use_autocast = DEVICE in ["cuda", "xpu", "cpu"]
            autocast_dtype = torch.float16 if DEVICE != "cpu" else torch.bfloat16
            i_p = 0
            with torch.no_grad():
                for y in y_coords:
                    for x in x_coords:
                        i_p += 1
                        if i_p % 10 == 0:
                            print(f"[SEGMENTATION] Progress: { ( i_p * 100 / ( len(x_coords) * len(y_coords) ) ) } %",end="\r")
                        if not should_run():
                            break
                        while should_halt():
                            time.sleep(0.125)

                        patch = image[y:y+IMG_WINDOW_SIZE, x:x+IMG_WINDOW_SIZE].copy()
                        input_tensor = torch.from_numpy(patch).to(device=DEVICE, dtype=torch.float32).permute(2, 0, 1) / 255.0

                        # Normalization pytorch tensor using the detected device
                        mean = torch.tensor([0.485, 0.456, 0.406], device=DEVICE).view(3, 1, 1)
                        std = torch.tensor([0.229, 0.224, 0.225], device=DEVICE).view(3, 1, 1)
                        input_tensor = (input_tensor - mean) / std

                        # Format to (1, C, H, W)
                        input_tensor = input_tensor.unsqueeze(0)

                        if use_autocast:
                            try:
                                with torch.amp.autocast(device_type=DEVICE, dtype=autocast_dtype):
                                    output = model(input_tensor)
                            except RuntimeError:
                                output = model(input_tensor)
                        else:
                            output = model(input_tensor)
                            
                        probabilities = torch.sigmoid(output)
                        prob_matrix = probabilities.squeeze().cpu().numpy()

                        patch_mask_uint8 = (prob_matrix > THRESHOLD).astype(np.uint8) * 255

                        p_h, p_w = patch_mask_uint8.shape
                        global_buildings_mask[y:y+p_h, x:x+p_w] = patch_mask_uint8
                    
                        # Send probability matrix to triangulate
                        global_vertices, global_indices, id_poly = triangulate_mask(
                            prob_mask = prob_matrix,
                            global_vertices = global_vertices,
                            global_indices = global_indices,
                            id_poly = id_poly_cnt,
                            offset = (x,y),
                            world_scale = px_res,
                            elevation_data = elevation_data,
                            threshold = THRESHOLD,
                            tolerance = 1.0
                        )

        tiflib.imwrite(output_path, global_buildings_mask)
        print(f"[SEGMENTATION_OUTPUT] Saved Building Mask at {output_path}")
        
    
    # Send accumulated building geometry to the rendering pipeline
    if global_vertices:
        print(f"\n[RENDERING] {len(global_vertices)} vertices, {len(global_indices) // 3} triangles")
        set_geometry(global_vertices, global_indices)
    else:
        print("\n[RENDERING] No buildings detected")

    # Test out GIS geometry
    #gis_vertices, gis_indices = extract_3d_buildings(lat_lon_bounds[2], lat_lon_bounds[0], lat_lon_bounds[3], lat_lon_bounds[1], 0.0)
    #
    #if gis_vertices:
    #    print(f"\n[RENDERING] {len(gis_vertices) // 3} vertices, {len(global_indices) // 3} triangles")
    #    set_geometry(gis_vertices, gis_indices)
    #else:
    #    print("\n[RENDERING] No buildings detected")
    print("[GIS] Extracting Building Heights from OSM...")
    height_raster_osmnx = extract_building_height_raster(
        lat_lon_bounds[2], lat_lon_bounds[0], 
        lat_lon_bounds[3], lat_lon_bounds[1], px_res[0], 0.0
    )
    print("[GIS] Extracting Building Heights from OSM (SUCCESS)")
    if height_raster_osmnx is not None:
        if len(height_raster_osmnx.shape) == 2:
            height_raster_osmnx = np.expand_dims(height_raster_osmnx, axis=-1)
        height_raster_osmnx = height_raster_osmnx.astype(np.float32).copy(order="C")
        push_texture_pixels(height_raster_osmnx, "osm_heights")

    # Send building mask to rendering pipeline as a GL_TEXTURE
    print("[GIS] Sending global textures from Python to GPU...")
    push_texture_pixels(image, "satellite")
    if len(global_buildings_mask.shape) == 2:
        global_buildings_mask = np.expand_dims(global_buildings_mask, axis=-1)
    final_mask_buildings = global_buildings_mask.astype(np.uint8).copy(order="C")
    push_texture_pixels(final_mask_buildings, "buildings_mask")
    center_elevation = elevation_matrix - avg_height
    # Force float32 format
    terrain_float32 = center_elevation.astype(np.float32)
    # Send the matrix as a GL_TEXTURE for elevation (heightmap)
    push_texture_pixels(terrain_float32, "heightmap")

    sent_px_cmd()

    push_tmy_data(tmy_tex, sun_tex)
    
    time.sleep(2)

    reshade()

def main():
    image_paths = GET_INRIA_TEST_IMG_PATHS()
    
    print(f"[SATELLITE_IMAGES] Image directory contains {len(image_paths)} images")
    
    for idx, img_path in enumerate(image_paths):
        if should_run():
            while should_halt():
                time.sleep(0.125)
            filename = os.path.basename(img_path)

            print(f"\n[SATELLITE_IMAGES] [{idx+1}/{len(image_paths)}] Processing {filename}...")
        
            try:
                process_and_triangulate_image(img_path)
            except Exception as e:
                print(f"[SATELLITE_IMAGES] Erorr processing {filename}: {str(e)}")

if __name__ == "__main__":
    main()
