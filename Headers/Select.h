#pragma once

#include "Texture.h"
#include "Camera.h"
#include "Engine3D.h"

struct WorldBBox {
	AVector3 WorldMin, WorldMax;
public:
	WorldBBox(AVector3 _WorldMin, AVector3 _WorldMax) : WorldMin(_WorldMin), WorldMax(_WorldMax) {}
};

class TexturePxSelecter {
	FBO_Sampler* fbo = nullptr;
public:
	TexturePxSelecter(FBO_Sampler* _fbo) : fbo(_fbo) {}
	void Select(Window* window, Camera* cam, WorldBBox* bbox, int mouseX, int mouseY, bool flip, float* out_pixel);
	
};