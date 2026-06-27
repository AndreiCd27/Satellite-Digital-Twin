
#pragma once

#include <mutex>

#include "GeometryBasics.h"

class SceneException : public std::runtime_error {
public:
	SceneException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

class Scene {
private:

	std::vector<AVertex> VBO_Vector;
	std::vector<GLuint> EBO_Vector;

	std::mutex sceneMutex;
	bool UpdateBuffers = false;

public:

	static bool SCENE_GEOMETRY_UPDATE;

	std::mutex& GetMutex() {
		return sceneMutex;
	}

	Scene() = default;

	// Copy and equal constructor are FORBIDEN
	// This is a design choice, beacause one clone of a Scene
	// With a big memory space (on the heap) may crash the program
	// Or trigger many heap reallocations
	// A new Scene may be added from zero, but right now Engine3D only has MainScene
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void SetGeometry(std::vector<AVertex>& vertices, std::vector<GLuint>& indicies);
	void PushGeometry(std::vector<AVertex>& add_vertices, std::vector<GLuint>& add_indicies);
	bool GetUpdateStatus();

	int GetVBOsize();
	int GetEBOsize();

	void PrintVBO();
	void PrintEBO();

	const std::vector<AVertex>& GetVBO_Vector();
	const std::vector<GLuint>& GetEBO_Vector();

	void ClearVBO();
	void ClearEBO();

	void ResetUpdateStatus();
};