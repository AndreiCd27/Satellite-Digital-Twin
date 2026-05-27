
#include "Camera.h"
#include "GeometryLoader.h"

Camera::Camera(AVector3 pos, float _Yaw, float _Pitch)
{
	//std::cout << "C -> Camera \n";
	Position = pos;
	Yaw = _Yaw;
	Pitch = _Pitch;
}

const GLfloat scalingFactor = 0.001f;

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

	glUniform3f(shader.GetUniformLocation("CamPosition"), Position.x, Position.y, Position.z);
	glUniformMatrix4fv(shader.GetUniformLocation("perspectiveMatrix"), 1, GL_FALSE, glm::value_ptr(perspMatrix));
}

void Camera::LightMatrix(float shadowMapScale, Shader& shader, bool TextureBias) {

	//Light matrix uniformID is already inside SunCamera (see class Engine3D)

	glm::vec3 lightPos = glm::vec3(this->Position.x, this->Position.y, this->Position.z);

	//////////////////////////////////////////////////////////////////////////////// CHECK THIS
	mat4Tuple.proj = glm::ortho(-shadowMapScale, shadowMapScale, -shadowMapScale, shadowMapScale, -1000.0f, 1000.0f);
	mat4Tuple.view = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 depthMatrix = mat4Tuple.proj * mat4Tuple.view;

	if (TextureBias) {
		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);
		depthMatrix = biasMatrix * depthMatrix;
		glUniformMatrix4fv(shader.GetUniformLocation("lightPerspMatrix"), 
			1, GL_FALSE, glm::value_ptr(depthMatrix)
		);
	} else {
		glUniformMatrix4fv(shader.GetUniformLocation("lightPerspMatrix"), 
			1, GL_FALSE, glm::value_ptr(depthMatrix)
		);
		glUniformMatrix4fv(shader.GetUniformLocation("perspectiveMatrix"), 
			1, GL_FALSE, glm::value_ptr(depthMatrix)
		);
		
	}
};

void Camera::Inputs(GLFWwindow* window)
{

	// Handles key inputs

	// ROTATION (Q, E, R, T)
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) Yaw -= sensitivity;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) Yaw += sensitivity;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) Pitch -= sensitivity;
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) Pitch += sensitivity;

	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;

	// Recalculate Rotation Vector
	AVector3 newRotation;
	newRotation.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	newRotation.y = sin(glm::radians(Pitch));
	newRotation.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

	this->Rotation = newRotation;

	// (W, A, S, D, I, O)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += Rotation * speed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position += Rotation * -speed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position += (Rotation ^ Up).Normalize() * -speed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += (Rotation ^ Up).Normalize() * speed;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) Position += Up * -speed;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) Position += Up * speed;

	// Avoid camera rolling
	if (Pitch > 89.0f) Pitch = 89.0f;
	if (Pitch < -89.0f) Pitch = -89.0f;
}