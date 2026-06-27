
import numpy as np
from skimage import measure
from shapely.geometry import Polygon
import mapbox_earcut as earcut

import py_engine3d  # C++ CRender Module
from py_engine3d import AVertex

def color_hash(gid):
	return max((gid * 291127) % 256,128), max((gid * 201093) % 256,128), max((gid * 1771013) % 256,128)

def triangulate_mask(prob_mask, global_vertices, global_indices, id_poly, offset, world_scale, elevation_data, threshold=0.5, tolerance=1.0):
	# Find contours
	contours = measure.find_contours(prob_mask, level=threshold)

	# Used for correct indexing for the indicies buffer in CRender
	idx_counter = len(global_vertices)

	# Elevation of buildings


	for contour in contours:

		id_poly += 1

		if len(contour) < 3:
			continue
		
		
		h_avg = elevation_data["average_height"]
		h_scale = elevation_data["heightmap_scale"]
		heightmap = elevation_data["elevation_matrix"]
		xmin, xmax, ymin, ymax = elevation_data["heightmap_bounds"]
		hrows, hcols = heightmap.shape

		coords = [((pt[1] + offset[0])*world_scale[0] + xmin, ymax - (pt[0] + offset[1])*world_scale[1]) for pt in contour]
		
		# Shaely reduces point count
		poly = Polygon(coords)
		# Simplify contours
		simplified_poly = poly.simplify(tolerance, preserve_topology=True)
		
		if simplified_poly.is_empty or not hasattr(simplified_poly, 'exterior'):
			continue
			
		# Extract simplified points
		poly_vertices = np.array(simplified_poly.exterior.coords[:-1], dtype=np.float32)
		
		if len(poly_vertices) < 3:
			continue

		# Triangulation using mapbox_earcut, winding order is CCW
		ring_end_indices = np.array([len(poly_vertices)], dtype=np.int32)
		local_indices = []
		try:
			local_indices = earcut.triangulate_float32(poly_vertices, ring_end_indices)
		except Exception as e:
			print(f"[TRIANGULATION WARNING] Corrupted building geometry: {e}")
			continue

		# Mapping to gloal coordinates for CRender

		nr_points = len(poly_vertices)
		r,g,b = color_hash(id_poly)
		# Building Base
		for pt in poly_vertices:
			global_vertices.append(AVertex(pt[0], 0, pt[1], r, g, b, 255, id_poly))
		# Building Roof
		for pt in poly_vertices:
			# Insert an ROOFTOP ID, such that in the render UV attribute becomes (1, id_poly) <-> id_poly + 65536
			global_vertices.append(AVertex(pt[0], 0, pt[1], r, g, b, 255, id_poly + 65536))
		
		# Indicies for roof after triangulation
		# Adjust indicies for the global CRender index buffer
		for idx in local_indices:
			global_indices.append(int(idx + idx_counter + nr_points))
		# Indicies for lateral faces
		for i in range(nr_points):
			next_i = (i + 1) % nr_points
            
			base_0 = idx_counter + i
			base_1 = idx_counter + next_i
			roof_0 = idx_counter + nr_points + i
			roof_1 = idx_counter + nr_points + next_i
            
			global_indices.append(base_0)
			global_indices.append(base_1)
			global_indices.append(roof_0)

			global_indices.append(roof_0)
			global_indices.append(base_1)
			global_indices.append(roof_1)
		
		idx_counter += (nr_points * 2)

	return global_vertices, global_indices, id_poly
