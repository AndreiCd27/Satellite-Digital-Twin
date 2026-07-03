
#include "Select.h"

void TexturePxSelecter::Select(Window* window, Camera* cam, WorldBBox* bbox, int mouseX, int mouseY, bool flip, float* out_pixel) {
	int screenWidth = window->getWidth();
	int screenHeight = window->getHeight();

	Texture* Tex = fbo->GetTexPtr();
	if (Tex == nullptr) {
		std::cout << "[FBO_ERROR] FBO not configured with any Texture\n";
		return; 
	}
	int textureWidth = Tex->GetWidth();
	int textureHeight = Tex->GetHeight();
	// Normalized Coordinates
	float normX = (2.0f * mouseX) / screenWidth - 1.0f;
	float normY = 1.0f - (2.0f * mouseY) / screenHeight;

	// Screen Space
	glm::vec4 rayClip = glm::vec4(normX, normY, -1.0f, 1.0f);
	glm::vec4 rayScreen = glm::inverse(cam->mat4Tuple.proj) * rayClip;
	rayScreen = glm::vec4(rayScreen.x, rayScreen.y, -1.0f, 0.0f);

	// Screen Space to World Space
	glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(cam->mat4Tuple.view) * rayScreen));
	glm::vec3 rayOrigin = glm::vec3(cam->Position.x, cam->Position.y, cam->Position.z);

	float planeY = 0.0f;
	float minX = bbox->WorldMin.x; float minZ = bbox->WorldMin.z;
	float maxX = bbox->WorldMax.x; float maxZ = bbox->WorldMax.z;

	if (glm::abs(rayDir.y) > 0.0001f) {
		float t = (planeY - rayOrigin.y) / rayDir.y;

		if (t >= 0.0f) {
			glm::vec3 intersectionPoint = rayOrigin + t * rayDir;

			float worldX = intersectionPoint.x;
			float worldZ = intersectionPoint.z;

			// Normalize the position between 0.0 and 1.0 (UV Coordinates)
			float u = (worldX - minX) / (maxX - minX);
			float v = (worldZ - minZ) / (maxZ - minZ);
			if (flip) v = 1.0f - v;

			if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {

				int texX = static_cast<int>(u * (textureWidth - 1));
				int texY = static_cast<int>(v * (textureHeight - 1));

				GLint previousFBO;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

				glBindFramebuffer(GL_FRAMEBUFFER, fbo->GetFBO_ID());

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Tex->GetTexID(), 0);
				glReadBuffer(GL_COLOR_ATTACHMENT0);

				glReadPixels(texX, texY, 1, 1, Tex->GetFormat(), Tex->GetType(), out_pixel);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
			}

		}
	}


}