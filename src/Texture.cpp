

#include "Texture.h"

Texture::Texture(int width, int height) : width(width), height(height) {};

void Texture::Delete() {
	glDeleteTextures(1, &TexID);
}

void Texture::GenTex2D() {

	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_2D, TexID);
}

void Texture::MinMagFilter(GLenum MIN_FILTER, GLenum MAX_FILTER) const {

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MIN_FILTER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MAX_FILTER);
}

void Texture::WrapFilter(GLenum WRAP_S, GLenum WRAP_T) const {

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WRAP_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WRAP_T);
}

void Texture::CreateTex2D(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) const {

	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, width, height, 0, Format, type, dataPTR);
}

void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) {
	GenTex2D();
	CreateTex2D(InternalFormat, Format, type, dataPTR);
}

void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, 
	GLenum MIN_FILTER, GLenum MAG_FILTER, void* dataPTR) {

	GenTex2D();

	MinMagFilter(MIN_FILTER, MAG_FILTER);

	CreateTex2D(InternalFormat, Format, type, dataPTR);

}

void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type,
	GLenum MIN_FILTER, GLenum MAG_FILTER, GLenum WRAP_S, GLenum WRAP_T, void* dataPTR) {

	GenTex2D();

	MinMagFilter(MIN_FILTER, MAG_FILTER);
	WrapFilter(WRAP_S, WRAP_T);

	CreateTex2D(InternalFormat, Format, type, dataPTR);

}

void ShadowSampler::setupFBO() {
	std::cout << "Setup FBO \n";

	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	FBO_ID = FramebufferName;
}

bool ShadowSampler::setupDepthTexture(const int SIZE) {

	//std::cout << "Setup Depth Texture \n";

	// Depth texture
	GenTex2D();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SIZE, SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, SIZE, SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	MinMagFilter(GL_LINEAR, GL_LINEAR);

	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	
	WrapFilter(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TexID, 0);

	glDrawBuffer(GL_NONE); // No color buffer is drawn to

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return false;
	return true;
}