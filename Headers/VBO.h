#pragma once

#include <glad/glad.h>

#include "GeometryBasics.h"

class VBO {
private:
	bool SetupComplete = false;
	GLuint ID;
	size_t Capacity = 0;
public:

	VBO() { 
		//std::cout << "C -> VBO \n"; 
	};

	void SetupFloatPTR(const float* vertices, GLsizeiptr size) {
		glGenBuffers(1, &ID);
		glBindBuffer(GL_ARRAY_BUFFER, ID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	};

	void Setup(std::vector<AVertex>& worldVertices, GLsizeiptr size, const int drawStyle);
	void Setup(const AVertex* PTR, GLsizeiptr size, const int drawStyle);
	void Bind();
	void Unbind();
	void Delete();

	inline bool GetCompleteStatus() { return SetupComplete; }

	GLuint GetID() const { return ID; };
};