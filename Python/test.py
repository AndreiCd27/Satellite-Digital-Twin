
import py_engine3d  # C++ CRender Module
from py_engine3d import AVertex
import numpy as np
import time
import math

def simple_geom(t):

	offset = math.sin(t * 5.0) * 0.2
	
	# Create a triangle
	sv = 20
	v1 = AVertex(0.0, (0.5 + offset) * sv, 0.0, 255, 0, 0, 255, 1)
	v2 = AVertex(-0.5 * sv, (-0.5 + offset) * sv, 0.0, 0, 255, 0, 255, 2)
	v3 = AVertex(0.5 * sv, (-0.5 + offset) * sv, 0.0, 0, 0, 255, 255, 3)
	
	vertices = [v1, v2, v3]
	indicies = [0, 1, 2]
	
	# Send data to C++ MutexQueue
	py_engine3d.set_geometry(vertices, indicies)

# Main Python loop
start_time = time.time()

try:
	while py_engine3d.should_run():
		current_time = time.time() - start_time
		
		simple_geom(current_time)
		time.sleep(0.016) # sleep for a time frame

except KeyboardInterrupt:
	print("[Python Thread] Script stopped.")
except Exception as e:
	print(f"[Python Thread Error]: {e}")
