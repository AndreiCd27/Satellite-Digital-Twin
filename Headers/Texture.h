#pragma once


#include "precompile.h"

#include "shaderClass.h"

class Texture {
protected:
	GLuint TexID;
    int width = 0, height = 0;

    void GenTex2D();
    void CreateTex2D(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR) const;

public:

    Texture() = default;
    Texture(int width, int height);

    void SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, void* dataPTR);
    void SetupTexture(GLint InternalFormat, GLenum Format, GLenum type,
        GLenum MIN_FILTER, GLenum MAG_FILTER, void* dataPTR);
    void SetupTexture(GLint InternalFormat, GLenum Format, GLenum type, 
        GLenum MIN_FILTER, GLenum MAG_FILTER, GLenum WRAP_S, GLenum WRAP_T, void* dataPTR);

    void MinMagFilter(GLenum MIN_FILTER, GLenum MAX_FILTER) const;
    void WrapFilter(GLenum WRAP_S, GLenum WRAP_T) const;

	GLuint GetTexID() const { return TexID; };

    int GetWidth() const{ return width; };
    int GetHeight() const{ return height; };

    void Delete();
};

class ShadowSampler : public Texture {
	GLuint FBO_ID;
public:
	void setupFBO();
	bool setupDepthTexture(const int SIZE);
	GLuint GetFBO_ID() const { return FBO_ID; };
};

template <int GL_FORMAT, int GL_COLOR_CHANNELS>
class Texture3D : public Texture {
    int depth = 0;
public:
    Texture3D(int _width, int _height, int _depth, bool GL_Linear) 
        : Texture(_width, _height), depth(_depth) {

        glGenTextures(1, &TexID);
        glBindTexture(GL_TEXTURE_3D, TexID);

        int levels = GetMaxLOD();

        glTexStorage3D(GL_TEXTURE_3D, levels, GL_FORMAT, width, height, depth);

        if (GL_Linear) {
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else {
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        }
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

    int GetMaxLOD() {
        return 1 + std::floor(std::log2(std::min({ width, height, depth })));
    }

    void Make_Mipmap() {
        glBindTexture(GL_TEXTURE_3D, TexID);
        glGenerateMipmap(GL_TEXTURE_3D);
    }

    void VerifyVoxelDataCPU(int gridX, int gridY, int gridZ) {

        std::vector<uint16_t> cpuData(gridX * gridY * gridZ, 0);

        glBindTexture(GL_TEXTURE_3D, TexID);

        // LOD = 0
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RED, GL_HALF_FLOAT, cpuData.data());
        glBindTexture(GL_TEXTURE_3D, 0);

        int activeVoxels = 0;

        for (int z = 0; z < gridZ; z++) {
            for (int y = 0; y < gridY; y++) {
                for (int x = 0; x < gridX; x++) {
                    int index = x + y * gridX + z * gridX * gridY;

                    if (cpuData[index] > 0) {
                        activeVoxels++;
                    }
                }
            }
        }

        std::cout << "\n>>> [CPU]: " << cpuData.size()
            << " VOXELS, non-empty VOXELS: " << activeVoxels << " at Level Of Detail 0\n";
    }
};
