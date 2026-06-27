
#include "shaderClass.h"

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename)
{
	try {
		std::ifstream in(filename, std::ios::binary);
		if (in)
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			contents.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();
			return(contents);
		}
		throw ShaderException("Shader file not found",0);
	}
	catch (ShaderException& e) {
		std::cout << e.what() << "\n";
	}
	return "";
}


// Activates the Shader Program
void AbstractShader::Activate()
{
	glUseProgram(ID);
}

// Deletes the Shader Program
void AbstractShader::Delete()
{
	glDeleteProgram(ID);
}

void AbstractShader::compileErrors(unsigned int shader, const char* type)
{
	// Stores status of compilation
	GLint hasCompiled;
	// Character array to store error message in
	char infoLog[1024];
	if (strcmp(type, "PROGRAM") != 0)
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_COMPILATION_ERROR for:" << type << "\n" << infoLog << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_LINKING_ERROR for:" << type << "\n" << infoLog << std::endl;
		}
	}
}

void AbstractShader::compileErrors(unsigned int shader, const char* type, std::string tag) {
	// Stores status of compilation
	GLint hasCompiled;
	// Character array to store error message in
	char infoLog[1024];
	if (strcmp(type, "PROGRAM") != 0)
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_COMPILATION_ERROR for:" << type << "\n" << infoLog << std::endl;
			std::cout << "[SHADER] Compile ERROR for " << tag << "\n";
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_LINKING_ERROR for:" << type << "\n" << infoLog << std::endl;
			std::cout << "[SHADER] Compile ERROR for " << tag << "\n";
		}
	}
}

// Constructor that build the Shader Program from 2 different shaders
void Shader::Setup(const char* vertexFile, const char* fragmentFile)
{
	//std::cout << "Starting shader setup \n";

	// Read vertexFile and fragmentFile and store the strings
	std::string vertexCode = get_file_contents(vertexFile);
	std::string fragmentCode = get_file_contents(fragmentFile);

	// Convert the shader source strings into character arrays
	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();

	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);
	// Checks if Shader compiled succesfully
	compileErrors(vertexShader, "VERTEX");

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);
	// Checks if Shader compiled succesfully
	compileErrors(fragmentShader, "FRAGMENT");

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(ID);
	// Checks if Shaders linked succesfully
	compileErrors(ID, "PROGRAM");

	//std::cout << "Shader Program compiled \n";

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	SetupComplete = true;

}

GLuint AbstractShader::GetUniformLocation(const std::string& uniformName) {
	if (uniforms.find(uniformName) == uniforms.end()) {
		uniforms.try_emplace(uniformName, glGetUniformLocation(ID, uniformName.c_str()));
	}
	return uniforms[uniformName];
}

void AbstractShader::SetUniformMatrix4by4(const std::string& uniformName, glm::mat4 Mat4) {
	glUniformMatrix4fv(GetUniformLocation(uniformName), 1, GL_FALSE, glm::value_ptr(Mat4));
}
void AbstractShader::SetUniformVector3(const std::string& uniformName, glm::vec3 Vec3) {
	glUniform3f(GetUniformLocation(uniformName), Vec3.x, Vec3.y, Vec3.z);
}
void AbstractShader::SetUniformVector3(const std::string& uniformName, AVector3 Vec3) {
	glUniform3f(GetUniformLocation(uniformName), Vec3.x, Vec3.y, Vec3.z);
}
void AbstractShader::SetUniformVector2(const std::string& uniformName, float x, float y) {
	glUniform2f(GetUniformLocation(uniformName), x, y);
}
void AbstractShader::SetUniformVector3_int(const std::string& uniformName, glm::ivec3 Vec3) {
	glUniform3i(GetUniformLocation(uniformName), Vec3.x, Vec3.y, Vec3.z);
}

void AbstractShader::SetUniformVec4Array(const std::string& uniformName, std::vector<float> F) {
	glUniform4fv(GetUniformLocation(uniformName), (GLsizei)(F.size() / 4), F.data());
}
void AbstractShader::SetUniformVec4Array(const std::string& uniformName, float* F, int Fsize) {
	glUniform4fv(GetUniformLocation(uniformName), (GLsizei)Fsize / 4, F);
}
void AbstractShader::SetUniformVec3Array(const std::string& uniformName, float* F, int Fsize) {
	glUniform3fv(GetUniformLocation(uniformName), (GLsizei)Fsize, F);
}

void AbstractShader::SetInt(const std::string& uniformName, int val) {
	glUniform1i(GetUniformLocation(uniformName), val);
}
void AbstractShader::SetUInt(const std::string& uniformName, unsigned int val) {
	glUniform1ui(GetUniformLocation(uniformName), val);
}
void AbstractShader::SetFloat(const std::string& uniformName, float val) {
	glUniform1f(GetUniformLocation(uniformName), val);
}