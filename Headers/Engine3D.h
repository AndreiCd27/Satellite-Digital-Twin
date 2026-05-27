#pragma once

#include "stl_reader.h" // .stl geometry file reader by sreiter https://github.com/sreiter/stl_reader

#include "shaderClass.h"
#include "GeometryOrganizer.h"
#include "VBO.h"
#include "EBO.h"
#include "VAO.h"
#include "Camera.h"
#include "Texture.h"

//bool _Engine3D_Started = false;

// SINGLETON ENGINE3D
class Engine3D {
private:

	static Engine3D* engine;

	// Initialize a scene and a camera
	Camera UserCamera = Camera(AVector3(0.0f, 0.0f, 0.0f),0.0f, 0.0f);
	Camera SunCamera = Camera(AVector3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
	Scene MainScene;

	Texture depthTextureObject;

	// GLFW window object
	GLFWwindow* window = nullptr;
	int windowHeight = 0;
	int windowWidth = 0;
	float windowAspectRatio = 0.0f;

	// Shader
	Shader shaderProgram;
	Shader instanceProgram;
	VAO VAO_1;
	VBO VBO_1;
	EBO EBO_1;

	GLuint instanceVBO;

	GLuint shadowMapLocation;

	GLuint lightDirUnifLoc;

	// Appearance
	struct {
		float R = 0.2f;
		float G = 0.3f;
		float B = 0.5f; 
		float A = 1.0f;
	} backgroundColor;

	void registerCameraInput(float FOVdeg, float zNear, float zFar);

	Engine3D() = default;

public:

	static Engine3D* GetEngine3D();

	~Engine3D() { EngineTerminate(); delete engine; };
	// SETERS

	int setupGLFW(const int WINDOW_WIDTH, const int WINDOW_HEIGHT, const char* WINDOW_TITLE);

	void setCamera(float posX, float posY, float posZ);
	void setSunCamera(float posX, float posY, float posZ);

	void setCamera(float posX, float posY, float posZ, float yaw, float pitch);

	const int getDrawStyle(const char* style);

	void setupShaders();

	void setupGeometryArrayObjects(const int drawStyle);

	void setupInstanceVBO();
	void DrawInstances(Blueprint* BLUEPRINT, Tile* TILE);

	//void setupShadowMap(const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT);

	inline void setBackground(float R, float G, float B, float A) { backgroundColor = { R,G,B,A }; };

	// GETTERS
	Camera& getCamera(bool Sun);

	Scene* getScene();

	GLFWwindow* getWindow() { return window; }

	inline int windowShouldClose() { 
		return glfwWindowShouldClose(window); 
	}
	
	inline Shader& getShader() { 
		return shaderProgram; 
	}

	//OTHERS

	void initGameFrame();

	void shadowPass();

	void renderPass(float FOVdeg, float zNear, float zFar);

	void DrawAllInstances();

	Tile* getVisibleCameraFrustum();

	//void loadChunks(Tile* from);

	void EngineTerminate();

	Blueprint* LoadSTLGeomFile(const char* filePath, float scale);
	

	Blueprint* CreatePrism(const std::vector<AVertex>& vertices, int VertexNumber, float height);
	Blueprint* CreateRectPrism(float length, float width, float height);
	Blueprint* CreateCube(float length);
	
	void DEBUG_showCameraVectors();
};