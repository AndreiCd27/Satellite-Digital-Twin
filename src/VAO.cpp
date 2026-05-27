

#include "VAO.h"

void VAO::Setup() {
	glGenVertexArrays(1, &ID);

	SetupComplete = true;
}

void VAO::LinkVBO(VBO& VBO, GLuint layout, GLuint numComp, GLenum type, GLsizei stride, bool NORMALIZED, void* offset) {
	VBO.Bind();
	glVertexAttribPointer(layout, numComp, type, NORMALIZED, stride, offset);
	glEnableVertexAttribArray(layout);
	VBO.Unbind();
}

void VAO::Bind() {
	glBindVertexArray(ID);
}

void VAO::Unbind() {
	glBindVertexArray(0);
}

void VAO::Delete() {
	glDeleteVertexArrays(1, &ID);
}