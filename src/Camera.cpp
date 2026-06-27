
#include "Camera.h"
#include "GeometryBasics.h"

Camera::Camera(AVector3 pos, float _Yaw, float _Pitch) : Position(pos)
{
	//std::cout << "C -> Camera \n";
	Yaw = _Yaw;
	Pitch = _Pitch;
}

void Camera::Matrix(float FOVdeg, float near, float far, float aspect, Shader& shader) {
	glm::vec3 front;
	front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	front.y = sin(glm::radians(Pitch));
	front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	glm::vec3 camDir = glm::normalize(front);

	glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

	mat4Tuple.view = glm::lookAt(glm::vec3(0.0f), camDir, camUp);
	mat4Tuple.proj = glm::perspective(glm::radians(FOVdeg), aspect, near, far);

	glm::mat4 perspMatrix = mat4Tuple.proj * mat4Tuple.view;

	shader.SetUniformVector3("CamPosition", Position);
	shader.SetUniformMatrix4by4("perspectiveMatrix", perspMatrix);
}

void Camera::LightMatrix(float shadowMapScale, Shader& shader, bool TextureBias, AVector3 lookAtFrom) {

	//Light matrix uniformID is already inside SunCamera (see class Engine3D)

	glm::vec3 lightDir = (glm::vec3)Position.Normalize();

	float distance = 128.0f; // Distance from UserCamera to SunCamera

	//////////////////////////////////////////////////////////////////////////////// CHECK THIS
	mat4Tuple.proj = glm::ortho(-shadowMapScale, shadowMapScale, -shadowMapScale, shadowMapScale, -1000.0f, 1000.0f);
	mat4Tuple.view = glm::lookAt((glm::vec3)lookAtFrom + lightDir * distance, 
		(glm::vec3)lookAtFrom, glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 depthMatrix = mat4Tuple.proj * mat4Tuple.view;

	if (TextureBias) {
		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);
		depthMatrix = biasMatrix * depthMatrix;
		shader.SetUniformMatrix4by4("lightPerspMatrix",depthMatrix);
	} else {
		shader.SetUniformMatrix4by4("lightPerspMatrix", depthMatrix);
		shader.SetUniformMatrix4by4("perspectiveMatrix", depthMatrix);
		
	}
};

void Camera::Inputs(GLFWwindow* window, float msPerFrame)
{

	// Handles key inputs
	const float msInc = msPerFrame / 5.0f;
	// ROTATION (Q, E, R, T)
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) Yaw -= sensitivity * msInc;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) Yaw += sensitivity * msInc;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) Pitch -= sensitivity * msInc;
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) Pitch += sensitivity * msInc;

	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;

	// Recalculate Rotation Vector
	AVector3 newRotation;
	newRotation.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	newRotation.y = sin(glm::radians(Pitch));
	newRotation.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

	this->Rotation = newRotation;

	// (W, A, S, D, I, O)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += Rotation * speed * msInc;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position += Rotation * -speed * msInc;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position += (Rotation ^ Up).Normalize() * -speed * msInc;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += (Rotation ^ Up).Normalize() * speed * msInc;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) Position += Up * -speed * msInc;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) Position += Up * speed * msInc;

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		auto t = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> duration = t - tstop;
		if (duration.count() > 1000.0f) {
			tstop = t;
			StopMotion = not StopMotion;
		}
	}

	// Avoid camera rolling
	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;
}

bool Camera::StopMotion = false;