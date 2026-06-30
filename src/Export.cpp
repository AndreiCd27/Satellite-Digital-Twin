
#include "Export.h"

using json = nlohmann::json;

float GeoExporter::GetLon(float gl_x) {
    float width = (float)geodata->GeoFile.width;
    float res = geodata->GeoFile.MetersPerPixel;
    float widthMeters = width * res;
    float normalizedX = (gl_x + (widthMeters * 0.5f)) / widthMeters;
    if (normalizedX < 0.0f) normalizedX = 0.0f;
    if (normalizedX > 1.0f) normalizedX = 1.0f;
    float deltaLon = geodata->BBox.lon1 - geodata->BBox.lon0;
    return geodata->BBox.lon0 + (normalizedX * deltaLon);
}

float GeoExporter::GetLat(float gl_z) {
    float height = (float)geodata->GeoFile.height;
    float res = geodata->GeoFile.MetersPerPixel;
    float heightMeters = height * res;
    float normalizedZ = (gl_z + (heightMeters * 0.5f)) / heightMeters;
    if (normalizedZ < 0.0f) normalizedZ = 0.0f;
    if (normalizedZ > 1.0f) normalizedZ = 1.0f;
    float deltaLat = geodata->BBox.lat1 - geodata->BBox.lat0;
    return geodata->BBox.lat1 - (normalizedZ * deltaLat);
}



void GeoExporter::ExportVBOtoGeoJSON(Engine3D* engine, const std::string& filename) {
	// Assuming you have your VBO bound
	glBindBuffer(GL_ARRAY_BUFFER, engine->GetVBO_ID());

	// Determine the size of your buffer data (e.g., 3 floats per vertex)
	GLint bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

	std::vector<AVertex> vertices(bufferSize / sizeof(AVertex));
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    int width = geodata->GeoFile.width;
    int height = geodata->GeoFile.height;
    float res = geodata->GeoFile.MetersPerPixel;

    // Initialize the root GeoJSON feature object
    json geojson = {
        {"type", "FeatureCollection"},
        {"metadata", {
            {"filename", geodata->GeoFile.filename},
            {"filepath", geodata->GeoFile.filepath},
            {"CRS", geodata->GeoFile.CRS},
            {"resolution", res},
            {"width", width},
            {"height", height}
        }},
        {"bbox", {
            geodata->BBox.lon0,
            geodata->BBox.lat0,
            geodata->BBox.lon1,
            geodata->BBox.lat1
        }},
        {"features", json::array()}
    };

    float AverageElevation = geodata->AverageElevation;

    for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
        const auto& v0 = vertices[i];
        const auto& v1 = vertices[i + 1];
        const auto& v2 = vertices[i + 2];

        uint32_t id0 = v0.UV.UV;
        uint32_t id1 = v1.UV.UV;
        uint32_t id2 = v2.UV.UV;
        if (id0 > 65536u && id1 > 65536u && id2 > 65536u) {
            // Triangle is BUILDING ROOF
            // We save building height (meters) separate from terrain elevation
            uint32_t original_id = id0 - 65536u;

            float building_height = (v0.NORMAL.y + v1.NORMAL.y + v2.NORMAL.y) / 3.0f;

            float total_altitude = (v0.POS.y + v1.POS.y + v2.POS.y) / 3.0f;

            float terrain_elevation = total_altitude - building_height;

            json triangleRing = json::array({
                { GetLon(v0.POS.x), GetLat(v0.POS.z), v0.POS.y },
                { GetLon(v1.POS.x), GetLat(v1.POS.z), v1.POS.y },
                { GetLon(v2.POS.x), GetLat(v2.POS.z), v2.POS.y },
                { GetLon(v0.POS.x), GetLat(v0.POS.z), v0.POS.y }
                });

            json feature = {
                {"type", "Feature"},
                {"properties", {
                    {"id", original_id},
                    {"type", "building_roof"},
                    {"building_height", building_height},
                    {"terrain_elevation", terrain_elevation + AverageElevation},
                    {"total_elevation", total_altitude + AverageElevation}
                }},
                {"geometry", {
                    {"type", "MultiPolygon"},
                    {"coordinates", json::array({ json::array({ triangleRing})})}
                }}
            };
            geojson["features"].push_back(feature);
        }
        else {
            // Triangle is Building Base OR Terrain
            float terrain_elevation = (v0.POS.y + v1.POS.y + v2.POS.y) / 3.0f;
            uint32_t original_id = id0; // ID for Building Base OR Terrain

            json triangleRing = json::array({
                { GetLon(v0.POS.x), GetLat(v0.POS.z), v0.POS.y },
                { GetLon(v1.POS.x), GetLat(v1.POS.z), v1.POS.y },
                { GetLon(v2.POS.x), GetLat(v2.POS.z), v2.POS.y },
                { GetLon(v0.POS.x), GetLat(v0.POS.z), v0.POS.y }
                });

            json feature = {
                {"type", "Feature"},
                {"properties", {
                    {"id", original_id},
                    {"type", (id0 == 1) ? "terrain" : "building_base"},
                    {"building_height", 0.0},
                    {"terrain_elevation", terrain_elevation + AverageElevation},
                    {"total_elevation", terrain_elevation + AverageElevation}
                }},
                {"geometry", {
                    {"type", "MultiPolygon"},
                    {"coordinates", json::array({ json::array({ triangleRing }) })}
                }}
            };
            if (id0 == 1) geojson["features"].push_back(feature); // Fetch only Terrain
        }
    }

    // Write the file to disk
    std::ofstream file(filename);
    if (file.is_open()) {
        file << geojson.dump(4);
        file.close();
        std::cout << "Successfully exported geometry to " << filename << std::endl;
    }
    else {
        std::cerr << "Error writing to file " << filename << std::endl;
    }
}