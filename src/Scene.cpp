
#include "Scene.h"

bool Scene::SCENE_GEOMETRY_UPDATE = false;

void Scene::SetGeometry(std::vector<AVertex>& vertices, std::vector<GLuint>& indicies) {
	std::cout << "[RENDER] Reset geometry\n";
	// Block scene mutex when inserting new geometry to make sure OpenGL does not crash
	std::lock_guard<std::mutex> lock(sceneMutex);

	int cnt_roof = 0;
	int cnt_base = 0;
	int cnt_terrain = 0;
	for (auto& v : vertices) {
		if (v.UV.UV > 65536) { cnt_roof++; }
		else { if (v.UV.UV == 1) { cnt_terrain++; } else { cnt_base++; } }
	}

	// No deep copies
	VBO_Vector = std::move(vertices);
	EBO_Vector = std::move(indicies);

	std::cout << "[GEOMETRY] " << cnt_roof << " roof vertices, " << cnt_base << " base vertices "
		<< cnt_terrain << " terrain vertices ----------------------------------\n";

	UpdateBuffers = true;
	SCENE_GEOMETRY_UPDATE = true;
}

void Scene::PushGeometry(std::vector<AVertex>& add_vertices, std::vector<GLuint>& add_indicies) {
	//std::cout << "Pushed geometry\n";
	// Block scene mutex when inserting new geometry to make sure OpenGL does not crash
	std::lock_guard<std::mutex> lock(sceneMutex);

	VBO_Vector.insert(VBO_Vector.end(), add_vertices.begin(), add_vertices.end());
	EBO_Vector.insert(EBO_Vector.end(), add_indicies.begin(), add_indicies.end());

	// Imediately clear resources
	add_vertices.clear();
	add_vertices.shrink_to_fit();
	add_indicies.clear();
	add_indicies.shrink_to_fit();

	UpdateBuffers = true;
}

bool Scene::GetUpdateStatus() {
	return UpdateBuffers;
}
void Scene::ResetUpdateStatus() {
	UpdateBuffers = false;
}

int Scene::GetVBOsize() {
	return (int)VBO_Vector.size();
}
int Scene::GetEBOsize() {
	return (int)EBO_Vector.size();
}

void Scene::PrintVBO() {
	for (const auto& v : VBO_Vector) {
		std::cout << "(X, Y, Z) = (" << v.POS.x << ", " << v.POS.y << ", " << v.POS.z << "), ID/UV = " << v.UV.UV << "\n";
	}
}
void Scene::PrintEBO() {
	for (const auto& i : EBO_Vector) {
		std::cout << "I = " << i << "\n";
	}
}

const std::vector<AVertex>& Scene::GetVBO_Vector() {
	return VBO_Vector;
}
const std::vector<GLuint>& Scene::GetEBO_Vector() {
	return EBO_Vector;
}

void Scene::ClearVBO() {
	// Clear the memory footprint of the VBO RAM copy
	// Call ONLY after the buffer was sent to VRAM
	VBO_Vector.clear();
	VBO_Vector.shrink_to_fit();
}
void Scene::ClearEBO() {
	// Clear the memory footprint of the VBO RAM copy
	// Call ONLY after the buffer was sent to VRAM
	EBO_Vector.clear();
	EBO_Vector.shrink_to_fit();
}
