#pragma once

#include "precompile.h"

class EBO {
private:
	bool SetupComplete = false;
	GLuint ID;
	size_t Capacity = 0;
public:

	EBO() { 
		//std::cout << "C -> EBO \n"; 
	};

	void Setup(std::vector<GLuint>& VertIndicies, GLsizeiptr size, const int drawStyle);
	void Setup(const GLuint* PTR, GLsizeiptr size, const int drawStyle);
	void Bind();
	void Unbind();
	void Delete();

	inline bool GetCompleteStatus() { return SetupComplete; }
};