import osmnx as ox
import numpy as np
from pyproj import Transformer
from shapely.geometry import Polygon

import py_engine3d  # C++ CRender Module
from py_engine3d import AVertex

def extract_3d_buildings(north, south, east, west, avg_height=0.0):
    try:
        # Download OSM buildings
        gdf = ox.features_from_bbox((north, south, east, west), tags={'building': True})
    except Exception as e:
        print(f"[GIS] Error downloading: {e}")
        return [], []

    if 'height' in gdf.columns:
        # Convert NaN to 0
        gdf['height'] = gdf['height'].fillna('0').astype(str)
    else:
        gdf['height'] = '0'

    if 'building:levels' in gdf.columns:
        gdf['building:levels'] = gdf['building:levels'].fillna('0').astype(str)
    else:
        gdf['building:levels'] = '0'

    try:
        # UTM Projection to meter scale
        gdf_projected = ox.projection.project_gdf(gdf)
    except Exception as e:
        print(f"[GIS] Error during projection (UTM conversion): {e}")
        return [], []
    
    # Local center
    centroid = gdf_projected.unary_union.centroid
    center_x, center_y = centroid.x, centroid.y

    global_vertices = []
    global_indices = []
    vertex_counter = 0

    id_poly = 65536

    for idx, row in gdf_projected.iterrows():
        height = 3.0
        if 'height' in row and not np.isnan(float(row['height']) if isinstance(row['height'], (int, float)) else 0):
            try: height = float(row['height'])
            except: pass
        elif 'building:levels' in row and not np.isnan(float(row['building:levels']) if isinstance(row['building:levels'], (int, float)) else 0):
            try: height = float(row['building:levels']) * 3.0
            except: pass

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
