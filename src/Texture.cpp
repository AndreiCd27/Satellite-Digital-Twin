

#include "Texture.h"

Texture::Texture(int width, int height) : width(width), height(height) {};

void Texture::Delete() {
	glDeleteTextures(1, &TexID);
}

void Texture::Clear() {
	glBindTexture(GL_TEXTURE_2D, TexID);

	int channels = 1;
	if (m_format == GL_RG || m_format == GL_RG_INTEGER) channels = 2;
	else if (m_format == GL_RGB || m_format == GL_RGB_INTEGER) channels = 3;
	else if (m_format == GL_RGBA || m_format == GL_RGBA_INTEGER) channels = 4;

	int bytesPerChannel = 1;
	if (m_type == GL_FLOAT || m_type == GL_UNSIGNED_INT || m_type == GL_INT) {
		bytesPerChannel = 4;
	}
	else if (m_type == GL_SHORT || m_type == GL_UNSIGNED_SHORT) {
		bytesPerChannel = 2;
	}

	size_t totalBytes = static_cast<size_t>(width) * height * channels * bytesPerChannel;

	std::vector<uint8_t> emptyData(totalBytes, 0);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, m_format, m_type, emptyData.data());
}


void Texture::GenTex2D() {

	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_2D, TexID);
}
void Texture::GenTex2D(int w, int h) {

	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_2D, TexID);
	width = w; height = h;
}

void Texture::MinMagFilter(GLenum MIN_FILTER, GLenum MAX_FILTER) const {

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MIN_FILTER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MAX_FILTER);
}

void Texture::WrapFilter(GLenum WRAP_S, GLenum WRAP_T) const {

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WRAP_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WRAP_T);
}

void Texture::CreateTex2D(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) {

	m_format = Format;
	m_internalFormat = InternalFormat;
	m_type = type;
	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, width, height, 0, Format, type, dataPTR);
}
void Texture::CreateTex2D(int w, int h, GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) {
	width = w;
	height = h;
	m_format = Format;
	m_internalFormat = InternalFormat;
	m_type = type;
	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, w, h, 0, Format, type, dataPTR);
}
void Texture::CreateStorageTex2D(int w, int h, GLint InternalFormat) {
	width = w;
	height = h;
	m_internalFormat = InternalFormat;

	m_format = GL_RED;

	glTexStorage2D(GL_TEXTURE_2D, 1, InternalFormat, w, h);
}


void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) {
	GenTex2D();
	m_format = Format;
	m_internalFormat = InternalFormat;
	m_type = type;
	CreateTex2D(InternalFormat, Format, type, dataPTR);
}

void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, 
	GLenum MIN_FILTER, GLenum MAG_FILTER, void* dataPTR) {

	GenTex2D();
	m_format = Format;
	m_internalFormat = InternalFormat;
	m_type = type;

	MinMagFilter(MIN_FILTER, MAG_FILTER);

	CreateTex2D(InternalFormat, Format, type, dataPTR);

}

void Texture::SetupTexture(GLint InternalFormat, GLenum Format, GLenum type,
	GLenum MIN_FILTER, GLenum MAG_FILTER, GLenum WRAP_S, GLenum WRAP_T, void* dataPTR) {

	GenTex2D();
	m_format = Format;
	m_internalFormat = InternalFormat;
	m_type = type;

	MinMagFilter(MIN_FILTER, MAG_FILTER);
	WrapFilter(WRAP_S, WRAP_T);

	CreateTex2D(InternalFormat, Format, type, dataPTR);

}

void Texture::BindImage(unsigned int unit, unsigned int access) const {
	glBindImageTexture(unit, this->TexID, 0, GL_FALSE, 0, access, m_internalFormat);
}

void ShadowSampler::setupFBO() {
	//std::cout << "Setup FBO \n";

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


void FBO_Sampler::attachTexture(Texture* _Tex) {
	Tex = _Tex;

	// Save previous active FBO
	GLint previousFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
	// Bind this FBO
	glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);

	// Attach Texture
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Tex->GetTexID(), 0);
	// Revert to previous FBO to preserve OpenGL state
	glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
}

void FBO_Sampler::genFBO() {

	glGenFramebuffers(1, &FBO_ID);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO_ID);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}