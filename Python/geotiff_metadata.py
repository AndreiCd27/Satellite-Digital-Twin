import rasterio
from rasterio.warp import transform_bounds
import os

def get_geotiff_metadata(filepath, silent=False):
    with rasterio.open(filepath) as src:

        res_x, res_y = src.res
        lon_min, lat_min, lon_max, lat_max = transform_bounds(src.crs, 'EPSG:4326', *src.bounds)

        if not silent:
            print("GeoTIFF dimensions: ")
            print(f"Width (px): {src.width}")
            print(f"Height (px): {src.height}")
            print(f"Bands: {src.count}")
            print(f"DataType: {src.dtypes}")

            print("\nGeoTIFF metadata: ")
            print(f"Coordinate system (CRS): {src.crs}")
            print(f"Bounds: {src.bounds}")
            print(f"Affine transform:\n{src.transform}")
            print(f"Pixel-Meter resolution: {res_x}, {res_y} (x,y)")

        
            print("\nGeographic coordinates (WGS84):")
            print(f"Latitude: {lat_min:.6f} - {lat_max:.6f}")
            print(f"Longitude: {lon_min:.6f} - {lon_max:.6f}")
        
        bounds = [lat_min, lat_max, lon_min, lon_max]

        crs_string = src.crs.to_proj4()
        filename = os.path.splitext(os.path.basename(filepath))[0]

        geodata = [lat_min, lon_min, lat_max, lon_max, res_x, src.width, src.height, filename, filepath, crs_string];

        return geodata, bounds, (res_x, res_y), (src.width, src.height)
