import time
import requests
import numpy as np

def get_topo_data(min_lat, max_lat, min_lon, max_lon, sample_size=20):
    '''
    Gets topological data from geographical coordinates (WGS84)
    sample_size determines at how many points an image is evaluated on X and Y axis
    For example, a value of 100 means 100x100 elevation points equaly distributed
    Outputs a standard matrix grid of elevation points
    '''
    print(f"[GIS] Extracting topological data for BBox: {min_lon},{min_lat}|{max_lon},{max_lat} (lon,lat)")
    # URL for OpenTopoData API
    url = "https://api.opentopodata.org/v1/srtm30m"
    # Numpy array to store elevation data

    elevation_matrix = np.zeros((sample_size, sample_size))

    points, matrix_idx = [], []

    for y in range(sample_size):
        for x in range(sample_size):
            dx = x / (sample_size - 1)
            dy = y / (sample_size - 1)

            lat = max_lat * (1 - dy) + min_lat * dy
            lon = min_lon * (1 - dx) + max_lon * dx
            
            points.append(f"{lat},{lon}")
            matrix_idx.append((y, x))
    # Maximum request size is 100 datapoints
    chunk_size = 100
    total_points = len(points)
    
    for i in range(0, total_points, chunk_size):
        chunk_points = points[i : i + chunk_size]
        chunk_indices = matrix_idx[i : i + chunk_size]
        
        locations_string = "|".join(chunk_points)
        data = {
            "locations": locations_string,
            "interpolation": "bilinear"
        }
        
        data_received = False
        while not data_received:
            try:
                response = requests.post(url, data=data, timeout=15)
                
                if response.status_code == 200:
                    data_received = True
                    results = response.json()["results"]
                    
                    # Map results to NumPy array
                    for idx, res in enumerate(results):
                        y_idx, x_idx = chunk_indices[idx]
                        elevation_matrix[y_idx, x_idx] = res["elevation"]
                    
                    print(f"[GIS] Downloaded {min(i + chunk_size, total_points)} / {total_points} points")
                    
                    # Sleep for 1.1 to avoid rate limit
                    time.sleep(1.1)
                    
                elif response.status_code == 429:
                    print("[GIS] Error code 429, too many requests")
                    time.sleep(5)
                else:
                    print(f"[GIS] API Error")
                    time.sleep(2)
                    
            except requests.exceptions.RequestException as e:
                print(f"[GIS] Connection error ({e})")
                time.sleep(3)

    avg_height = np.mean(elevation_matrix)
    print(f"[GIS] Average Terrain Height: {avg_height:.2f} meters")

    return elevation_matrix, avg_height

