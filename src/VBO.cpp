
#include "VBO.h"

void VBO::Setup(std::vector<AVertex>& worldVertices, GLsizeiptr size, const int drawStyle) {
	//std::cout << "Setup VBO!!!!\n";
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, worldVertices.data(), drawStyle);
	Capacity = size;
	//std::cout << "Setup VBO complete!!!! \n";
	SetupComplete = true;
}

void VBO::Setup(const AVertex* PTR, GLsizeiptr size, const int drawStyle) {
	//std::cout << "Setup VBO!!!!\n";
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, PTR, drawStyle);
	Capacity = size;
	//std::cout << "Setup VBO complete!!!! \n";
}

void VBO::Bind() {
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::Unbind() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete() {
	glDeleteBuffers(1, &ID);
}