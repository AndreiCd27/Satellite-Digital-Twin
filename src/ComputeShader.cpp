
#include "ComputeShader.h"

void ComputeShader::Setup(const char* computeFileName) {

	std::string ShaderCode = get_file_contents(computeFileName);
	// Convert the shader source strings into character arrays
	const char* ShaderSource = ShaderCode.c_str();

	// Create Shader Object and get its reference
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	// Attach Shader source to the Shader Object
	glShaderSource(computeShader, 1, &ShaderSource, NULL);
	// Compile the Shader into machine code
	glCompileShader(computeShader);
	// Checks if Shader compiled succesfully
	compileErrors(computeShader, "COMPUTE");

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Compute Shader to the Shader Program
	glAttachShader(ID, computeShader);
	// Link the shader into the Shader Program
	glLinkProgram(ID);
	// Checks if Shader linked succesfully
	compileErrors(ID, "PROGRAM");

	// Delete the now useless Shader object
	glDeleteShader(computeShader);

	SetupComplete = true;
}