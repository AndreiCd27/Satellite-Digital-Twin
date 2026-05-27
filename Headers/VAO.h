#pragma once

#include "precompile.h"

#include "VBO.h"

class VAO {
public:
	GLuint ID;
	VAO() { std::cout << "C -> VAO \n"; };

	void Setup();
	void LinkVBO(VBO& VBO, GLuint layout, GLuint numComp, GLenum type, GLsizei stride, bool NORMALIZED, void* offset);
	void Bind();
	void Unbind();
	void Delete();
};