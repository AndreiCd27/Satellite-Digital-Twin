#pragma once

#include "Texture.h"
#include "ComputeShader.h"

class Raymarcher {
	ComputeShader raymarchCompute;
public:
	Raymarcher() {
		raymarchCompute.Setup(("CRenderExtensions/ShadowAnalysis/ComputeShaders/raymarch_scene.comp"), "Raymarcher");
	}
	void RaymarchUnifiedScene(float optimal_azimuth, float optimal_elevation,
		Texture* unified_heights, Texture* simulated_shadow_mask, int groupX, int groupY);
};