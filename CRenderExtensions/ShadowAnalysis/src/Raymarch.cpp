
#include "Raymarch.h"

void Raymarcher::RaymarchUnifiedScene(float optimal_azimuth, float optimal_elevation,
	Texture* unified_heights, Texture* simulated_shadow_mask, int groupX, int groupY) {

	raymarchCompute.Activate();

    raymarchCompute.SetFloat("ShadowAngle", optimal_azimuth);
    raymarchCompute.SetFloat("ElevationAngle", optimal_elevation);
    raymarchCompute.SetFloat("MetersPerPixel", 0.3f);
    raymarchCompute.SetInt("MaxRaySteps", 120);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, unified_heights->GetTexID());

    simulated_shadow_mask->BindImage(1, GL_WRITE_ONLY);

    glDispatchCompute(groupX, groupY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}