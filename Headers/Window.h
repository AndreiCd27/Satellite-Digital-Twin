
#include "precompile.h"

class Window {
	// GLFW window object
	GLFWwindow* window = nullptr;
	int height = 0;
	int width = 0;
	float aspectRatio = 0.0f;
public:

	bool CreateWindow(int WINDOW_WIDTH, int WINDOW_HEIGHT, const char* WINDOW_TITLE);

	void Terminate();

	GLFWwindow* getWindow() { return window; }

	inline int windowShouldClose() {
		return glfwWindowShouldClose(window);
	}

	inline float getAspectRatio() {
		return aspectRatio;
	}
	inline int getHeight() {
		return height;
	}
	inline int getWidth() {
		return width;
	}
};