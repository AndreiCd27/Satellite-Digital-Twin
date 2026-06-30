
#include "Export.h"

using json = nlohmann::json;

void GeoExporter::ExportVBOtoGeoJSON(Engine3D* engine, const std::string& filename) {
	// Assuming you have your VBO bound
	glBindBuffer(GL_ARRAY_BUFFER, engine->GetVBO_ID());

	// Determine the size of your buffer data (e.g., 3 floats per vertex)
	GLint bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

	std::vector<AVertex> vertices(bufferSize / sizeof(AVertex));
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Initialize the root GeoJSON feature object
    json geojson = {
        {"type", "FeatureCollection"},
        {"metadata", {
            {"filename", geodata->GeoFile.filename},
            {"filepath", geodata->GeoFile.filepath},
            {"CRS", geodata->GeoFile.CRS},
            {"resolution", geodata->GeoFile.MetersPerPixel},
            {"width", geodata->GeoFile.width},
            {"height", geodata->GeoFile.height}
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
                { v0.POS.x, v0.POS.z, v0.POS.y },
                { v1.POS.x, v1.POS.z, v1.POS.y },
                { v2.POS.x, v2.POS.z, v2.POS.y },
                { v0.POS.x, v0.POS.z, v0.POS.y }
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
                { v0.POS.x, v0.POS.z, v0.POS.y },
                { v1.POS.x, v1.POS.z, v1.POS.y },
                { v2.POS.x, v2.POS.z, v2.POS.y },
                { v0.POS.x, v0.POS.z, v0.POS.y }
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
            geojson["features"].push_back(feature);
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