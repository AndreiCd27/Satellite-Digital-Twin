
#include "EBO.h"

void EBO::Setup(std::vector<GLuint>& VertIndicies, GLsizeiptr size, const int drawStyle) {
	//std::cout << "Setup EBO!!!!\n";
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, VertIndicies.data(), drawStyle);
	Capacity = size;
	//std::cout << "Setup EBO complete!!!!\n";
}
void EBO::Setup(const GLuint* PTR, GLsizeiptr size, const int drawStyle) {
	//std::cout << "Setup EBO!!!!\n";
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, PTR, drawStyle);
	Capacity = size;
	//std::cout << "Setup EBO complete!!!!\n";
	SetupComplete = true;
}

void EBO::Bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::Unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::Delete() {
	glDeleteBuffers(1, &ID);
}