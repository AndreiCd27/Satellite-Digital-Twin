#pragma once


#include "precompile.h"

#include "GeometryBasics.h"

#include "shaderClass.h"

class ComputeShader : public AbstractShader {
private:
	std::string tag = "No tag";
public:
	void Setup(const char* computeFileName);
	void Setup(const char* computeFileName, std::string tag);

	GLuint CreateSSBO() {
		GLuint ssboID;
		glGenBuffers(1, &ssboID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
		return ssboID;
	}

	template <typename T>
	void SetDataSSBO(const std::vector<T>& data, const int BufferSize, const GLuint SSBO_ID) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, BufferSize * sizeof(T), data.data(), GL_DYNAMIC_DRAW);
	}

	template <typename T>
	void AllocateEmptySSBO(const int BufferSize, const GLuint SSBO_ID) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_ID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, BufferSize * sizeof(T), NULL, GL_DYNAMIC_DRAW);
	}

	template <int BINDING>
	void BindSSBO(const GLuint SSBO_ID) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING, SSBO_ID);
	}

	template <typename T>
	void ResetSSBO(const int BufferSize, const GLuint SSBO_ID) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_ID);
		unsigned int* ptr = (unsigned int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, BufferSize * sizeof(T), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		if (ptr) {
			std::fill_n(ptr, BufferSize, 0);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	template <typename T>
	void ClearSSBO(const int BufferSize, const GLuint SSBO_ID) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO_ID);

		uint32_t zero = 0;
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
};