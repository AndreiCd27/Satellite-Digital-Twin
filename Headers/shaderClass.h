#pragma once


#include "precompile.h"

#include "GeometryBasics.h"

class ShaderException : public std::runtime_error {
public:
	ShaderException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ SHADER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ SHADER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

std::string get_file_contents(const char* filename);

class Shader {
private:
	std::unordered_map<std::string, GLuint> UNIFORM_LOCATIONS;
public:
	GLuint ID;
	Shader() = default;
	void Setup(const char* vertFileName, const char* fragFileName);
	void Activate();
	void Delete();
	// Checks if the different Shaders have compiled properly
	void compileErrors(unsigned int shader, const char* type);

	// Uniform methods
	const GLuint GetUniformLocation(const std::string& uniformName);

	void SetUniformMatrix4by4(const std::string& uniformName, glm::mat4 Mat4);
	void SetUniformVector3(const std::string& uniformName, glm::vec3 Vec3);
	void SetUniformVector3(const std::string& uniformName, AVector3 Vec3);
};