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

class AbstractShader {
protected:
	bool SetupComplete = false;
	GLuint ID;

	std::unordered_map<std::string, GLuint> uniforms;
public:
	virtual ~AbstractShader() = default;

	void Activate();
	void Delete();
	// Checks if the different Shaders have compiled properly
	void compileErrors(unsigned int shader, const char* type);
	void compileErrors(unsigned int shader, const char* type, std::string tag);

	inline bool GetCompleteStatus() { return SetupComplete; }

	// Uniform methods
	GLuint GetUniformLocation(const std::string& uniformName);

	void SetUniformMatrix4by4(const std::string& uniformName, glm::mat4 Mat4);
	void SetUniformVector3(const std::string& uniformName, glm::vec3 Vec3);
	void SetUniformVector3_int(const std::string& uniformName, glm::ivec3 Vec3);
	void SetUniformVector3(const std::string& uniformName, AVector3 Vec3);
	void SetUniformVector2(const std::string& uniformName, float x, float y);
	void SetUniformVec4Array(const std::string& uniformName, std::vector<float> F);
	void SetUniformVec4Array(const std::string& uniformName, float* F, int Fsize);
	void SetUniformVec3Array(const std::string& uniformName, float* F, int Fsize);
	void SetInt(const std::string& uniformName, int val);
	void SetUInt(const std::string& uniformName, unsigned int val);
	void SetFloat(const std::string& uniformName, float val);
};

class Shader : public AbstractShader {
private:
public:
	Shader() = default;
	void Setup(const char* vertFileName, const char* fragFileName);
};