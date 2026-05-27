#pragma once

#include "precompile.h"

#include "VBO.h"

class VAO {
private:
	bool SetupComplete = false;
	GLuint ID;
public:

	VAO() { 
		//std::cout << "C -> VAO \n"; 
	};

	void Setup();
	void LinkVBO(VBO& VBO, GLuint layout, GLuint numComp, GLenum type, GLsizei stride, bool NORMALIZED, void* offset);
	void Bind();
	void Unbind();
	void Delete();

	inline bool GetCompleteStatus() { return SetupComplete; }
};