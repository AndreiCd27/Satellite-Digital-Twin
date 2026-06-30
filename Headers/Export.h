#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "precompile.h"
#include "Engine3D.h"

#include "nlohmann/json.hpp" // Header-only JSON library

struct GeoTIFF {
	std::string filename = "N/A";
	std::string filepath;
	std::string CRS;
	float MetersPerPixel;
	int width, height;
public:
	GeoTIFF(float MetersPerPixels, int width, int height,
		const std::string& filename, const std::string& filepath, const std::string& CRS) :
		filename(filename), filepath(filepath), CRS(CRS), MetersPerPixel(MetersPerPixel), width(width), height(height)
	{ }
	GeoTIFF() = default;
};

struct GeoBBox {
	float lat0, lon0, lat1, lon1;
public:
	GeoBBox(float latitude0, float longitude0, float latitude1, float longitude1) :
		lat0(latitude0), lon0(longitude0), lat1(latitude1), lon1(longitude1) { }
	GeoBBox() = default;
};

struct GeoData {
	GeoBBox BBox;
	GeoTIFF GeoFile;
	float AverageElevation = 0.0f;
public:
	GeoData(const GeoBBox& bbox, const GeoTIFF& geofile, float AverageElevation) : BBox(bbox), GeoFile(geofile),
		AverageElevation(AverageElevation) {}
	GeoData(float lat0, float lon0, float lat1, float lon1, float MetersPerPixels, int width, int height,
		const std::string& filename, const std::string& filepath, const std::string& CRS, float AverageElevation) :
		BBox(lat0, lon0, lat1, lon1), GeoFile(MetersPerPixels, width, height, filename, filepath, CRS),
		AverageElevation(AverageElevation) {}
	GeoData() = default;
};

class GeoExporter {
	GeoData* geodata = nullptr;
public:
	GeoExporter(GeoData* geodata) : geodata(geodata) {}
	float GetLon(float gl_x);
	float GetLat(float gl_z);
	void ExportVBOtoGeoJSON(Engine3D* engine, const std::string& filename);
};