import osmnx as ox
import numpy as np
from pyproj import Transformer
from shapely.geometry import Polygon, MultiPolygon

import rasterio
from rasterio.features import rasterize
from affine import Affine

import py_engine3d  # C++ CRender Module
from py_engine3d import AVertex

def get_osm_gdf(north, south, east, west):
    try:
        # Download OSM buildings as GeoDataFrame
        gdf = ox.features_from_bbox((north, south, east, west), tags={'building': True})
        return gdf
    except Exception as e:
        print(f"[GIS] Error downloading: {e}")
        return []

def rectify_heights(gdf):
    if 'height' in gdf.columns:
        # Convert NaN to 0
        gdf['height'] = gdf['height'].fillna('0').astype(str)
    else:
        gdf['height'] = '0'

    if 'building:levels' in gdf.columns:
        gdf['building:levels'] = gdf['building:levels'].fillna('0').astype(str)
    else:
        gdf['building:levels'] = '0'


def b_height(row):
    if 'height' in row and str(row['height']) != 'nan':
        val = row['height']
        if isinstance(val, (list, np.ndarray)):
            val = str(val[0])
        else:
            val = str(val)
            
        val = val.replace('m', '').strip()
        try: 
            return float(val)
        except (ValueError, TypeError): 
            pass

    if 'building:levels' in row and str(row['building:levels']) != 'nan':
        val_lvl = row['building:levels']
        if isinstance(val_lvl, (list, np.ndarray)):
            val_lvl = val_lvl[0]
        try: 
            scalar_lvl = float(np.asarray(val_lvl).item())
            return scalar_lvl * 3.0
        except (ValueError, TypeError): 
            pass

    return 0.0

def UTM_proj(gdf):
    try:
        # UTM Projection to meter scale
        gdf_projected = ox.projection.project_gdf(gdf)
        return gdf_projected
    except Exception as e:
        print(f"[GIS] Error during projection (UTM conversion): {e}")
        return []

def extract_3d_buildings(north, south, east, west, avg_height=0.0):
    
    gdf = get_osm_gdf(north, south, east, west)

    rectify_heights(gdf)

    gdf_projected = UTM_proj(gdf)
    
    # Local center
    centroid = gdf_projected.unary_union.centroid
    center_x, center_y = centroid.x, centroid.y

    global_vertices = []
    global_indices = []
    vertex_counter = 0

    id_poly = 65536

    for idx, row in gdf_projected.iterrows():
        height = b_height(row)

        # Process Polygon Geometry
        if isinstance(row['geometry'], Polygon) and not row['geometry'].is_empty:
            exterior_coords = list(row['geometry'].exterior.coords)
            
            poly_verts = [(pt[0] - center_x, pt[1] - center_y) for pt in exterior_coords[:-1]]
            n_points = len(poly_verts)
            if n_points < 3:
                continue

            # Make Vertex Array
            base_vertex_start = vertex_counter
            
            for vx, vy in poly_verts:
                # Ground Vertex
                ground_v = AVertex(vx, 0.0, vy, 255, 255, 255, 255, id_poly)
                global_vertices.append(ground_v)
                # Roof Vertex
                roof_v = AVertex(vx, 0.0, vy, 255, 255, 255, 255, id_poly)
                global_vertices.append(roof_v)
                vertex_counter += 2

            for i in range(n_points):
                next_i = (i + 1) % n_points
                
                bottom_left  = base_vertex_start + (i * 2)
                top_left     = bottom_left + 1
                bottom_right = base_vertex_start + (next_i * 2)
                top_right    = bottom_right + 1

                global_indices.extend([bottom_left, top_left, bottom_right])
                global_indices.extend([top_left, top_right, bottom_right])

            # Make roof indicies
            roof_indices = []
            for i in range(1, n_points - 1):
                idx0 = base_vertex_start + 1
                idx1 = base_vertex_start + (i * 2) + 1
                idx2 = base_vertex_start + ((i + 1) * 2) + 1
                global_indices.extend([idx0, idx1, idx2])

    print("[GIS] Sending vertices from OverpassAPI to Main Python Script")
    return global_vertices, global_indices


def extract_building_height_raster(north, south, east, west, px_res=1.0, avg_height=0.0):
    
    gdf = get_osm_gdf(north, south, east, west)

    rectify_heights(gdf)

    gdf_projected = UTM_proj(gdf)

    bounds = gdf_projected.total_bounds
    minx, miny, maxx, maxy = bounds
    
    width = int(np.ceil((maxx - minx) / px_res))
    height = int(np.ceil((maxy - miny) / px_res))

    Atransform = Affine(px_res, 0.0, minx, 0.0, -px_res, maxy)
    shapes = []

    for idx, row in gdf_projected.iterrows():
        BuildingH = b_height(row)
        geom = row.geometry
        if isinstance(geom, (Polygon, MultiPolygon)):
            if geom.is_valid:
                shapes.append((geom, BuildingH))
    if not shapes:
        print("[GIS] Error: Building footprints not found for region given")
        print(f"[GIS] Error Region - N:{north}, S:{south}, E:{east}, W{west}")
        return None, None


    heightmap = rasterize(shapes=shapes, out_shape=(height, width), transform=Atransform,
        fill=0.0, all_touched=True, dtype=np.float32
    )

    # 2D Heightmap Texture
    return heightmap