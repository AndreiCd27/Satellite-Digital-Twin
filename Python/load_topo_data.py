
from geotiff_metadata import get_geotiff_metadata
from get_topography import get_topo_data
import time

import numpy as np

import py_engine3d  # C++ CRender Module
from py_engine3d import AVertex

def pos_ij(i, j, sample_size, xmin, xmax, ymin, ymax):
	dx, dy = j / (sample_size-1), i / (sample_size-1)
	return (xmin * (1 - dx) + xmax * dx, ymax * (1 - dy) + ymin * dy)


def get_elevation_matrix(filepath):
	
	heightmap_size = 20
	metadata, bounds, px_res, img_dim = get_geotiff_metadata(filepath, silent=True)

	meter_width = int(px_res[0] * img_dim[0])
	meter_height = int(px_res[1] * img_dim[1])

	xmin, xmax = -meter_width // 2, meter_width // 2
	ymin, ymax = -meter_height // 2, meter_height // 2

	elevation_matrix, avg_height = get_topo_data(*bounds, heightmap_size)

	return elevation_matrix, avg_height, heightmap_size, (xmin, xmax, ymin, ymax), bounds

def build_quad(global_indices, idx_counter, top_left, top_right, bottom_left, bottom_right):
	global_indices.append(top_left + idx_counter)
	global_indices.append(bottom_left + idx_counter)
	global_indices.append(top_right + idx_counter)

	global_indices.append(top_right + idx_counter)
	global_indices.append(bottom_left + idx_counter)
	global_indices.append(bottom_right + idx_counter)

def topo_geom(global_vertices, global_indices, elevation_data, silent=False):

	heightmap_size = elevation_data["heightmap_scale"]
	avg_height = elevation_data["average_height"]
	xmin, xmax, ymin, ymax = elevation_data["heightmap_bounds"]
	elevation_matrix = elevation_data["elevation_matrix"]
	# Used for correct indexing for the indicies buffer in CRender
	idx_counter = len(global_vertices)

	for i in range(0,heightmap_size):
		for j in range(0,heightmap_size):
			pos = pos_ij(i, j, heightmap_size, xmin, xmax, ymin, ymax)
			h = elevation_matrix[i][j]
			if h < 0.01:
				# h - avg_height
				global_vertices.append(AVertex(pos[0], h - avg_height, pos[1], 100, 100, 255, 255, 1))
			else:
				# h - avg_height
				global_vertices.append(AVertex(pos[0], h - avg_height, pos[1], 0, 160, 0, 255, 1))
			if i != heightmap_size-1 and j != heightmap_size-1:
				# Inserting quad (as 2 triangles)
				top_left = i * heightmap_size + j
				top_right = i * heightmap_size + j + 1
				bottom_left = (i + 1) * heightmap_size + j
				bottom_right = (i + 1) * heightmap_size + j + 1

				build_quad(global_indices, idx_counter, top_left, top_right, bottom_left, bottom_right)

	return global_vertices, global_indices
