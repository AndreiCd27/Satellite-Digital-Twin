#pragma once

#include "precompile.h"

#include "shaderClass.h"
#include "GeometryBasics.h"

// Here we store our view and projection matrix and we forward it to Main.cpp
// Such that we only store the resulting matrix for each instance,
// Without looking for uniforms for our shaders every frame
struct Mat4Pair {
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
};

class Camera
{
public:
	// Stores the main vectors of the camera
	AVector3 Position;
	AVector3 Rotation = {0.0f, 0.0f, -1.0f};
	AVector3 Up = { 0.0f, 1.0f, 0.0f };
	float Yaw, Pitch;
	Mat4Pair mat4Tuple;

	float speed = 0.2f;
	float sensitivity = 0.1f;

	// Camera constructor to set up initial values
	Camera(AVector3 pos, float _Yaw, float _Pitch);

	// Sends the camera perspective matrix to the Vertex Shader
	void Matrix(float FOVdeg, float nearPlane, float farPlane, float aspectRatio, Shader& shader);

	void LightMatrix(float shadowMapScale, Shader& shader, bool TextureBias, AVector3 lookAtFrom);
	// Handles camera inputs
	void Inputs(GLFWwindow* window, float msPerFrame);
};