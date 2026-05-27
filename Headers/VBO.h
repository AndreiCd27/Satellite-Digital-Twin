#pragma once

#include <glad/glad.h>

#include "GeometryBasics.h"

class VBO {
public:
	GLuint ID;
	size_t Capacity = 0;
	VBO() { std::cout << "C -> VBO \n"; };

	void Setup(std::vector<AVertex>& worldVertices, GLsizeiptr size, const int drawStyle);
	void Setup(const AVertex* PTR, GLsizeiptr size, const int drawStyle);
	void Bind();
	void Unbind();
	void Delete();
};