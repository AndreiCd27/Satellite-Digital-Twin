
#include "Window.h"

bool Window::CreateWindow(int WINDOW_WIDTH, int WINDOW_HEIGHT, const char* WINDOW_TITLE) {
	// INITIALIZE GLFW
	glfwInit();
	// SOME SPECIFICATIONS FOR OUR OPENGL VERSION THAT WE SEND TO GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//CORE profile from OPENGL; only modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//Create the WINDOW OBJECT with our defined width and height and title
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	//Error checking if anything went wrong in the window creating process
	if (window == nullptr) {
		std::cout << "Error when creating window \n";
		return false;
	}
	//Tell GLFW we are using our created window as it's context
	glfwMakeContextCurrent(window);

	width = WINDOW_WIDTH;
	height = WINDOW_HEIGHT;
	aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	return true;
}

void Window::Terminate() {
	if (window != nullptr) {
		glfwDestroyWindow(window);
		//Terminate GLFW
		glfwTerminate();
	}
}
