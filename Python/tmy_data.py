import pvlib
import numpy as np
import time

def get_tmy_data(lat, lon):
    tmy_tex, sun_tex = [], []
    tmy_loaded = False

    while not tmy_loaded:
        try:
            tmy_data, meta = pvlib.iotools.get_pvgis_tmy(lat, lon, map_variables=True)
            altitude = meta["inputs"]["location"]["elevation"]
            
            # Compute solar positions
            sol_pos = pvlib.solarposition.get_solarposition(
                time=tmy_data.index,
                latitude=lat, longitude=lon,
                altitude=altitude
            )
            
            azimuth_rad = np.radians(sol_pos["azimuth"].values)
            elevation_rad = np.radians(sol_pos["elevation"].values)
            
            sun_x = np.cos(elevation_rad) * np.sin(azimuth_rad)
            sun_y = np.sin(elevation_rad)  # Y = Up-vector
            sun_z = np.cos(elevation_rad) * -np.cos(azimuth_rad) 

            tmy_stacked = np.stack([
                tmy_data["ghi"].values,
                tmy_data["dni"].values,
                tmy_data["dhi"].values,
                tmy_data["temp_air"].values
            ], axis=-1).astype("float32")
    
            sun_stacked = np.stack([
                sun_x,
                sun_y,
                sun_z,
                elevation_rad
            ], axis=-1).astype("float32")

            tmy_tex = tmy_stacked.flatten()
            sun_tex = sun_stacked.flatten()

            tmy_loaded = True

        except Exception as e:
            print(f"[TMY_DATA_ERROR] {str(e)}")
            print(f"[TMY_DATA] Retry after 2 seconds")
            time.sleep(2)

    return tmy_tex, sun_tex
