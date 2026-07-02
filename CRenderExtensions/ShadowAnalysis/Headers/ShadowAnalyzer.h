#pragma once


#include "precompile.h"
#include "Engine3D.h"
#include "Raymarch.h"
#include "CommandBuffer.h"
#include <memory>
#include <vector>
#include <utility>

const std::string path = "CRenderExtensions/ShadowAnalysis";

class SatelliteAnalyzer {

    bool AnalysisDone = true;

    const int targetWidth, targetHeight;

    const float __PI = 3.14159265359f;

    Raymarcher RayMarchOperation;

    // Compute Shader for all stages
	ComputeShader DirectionalAnalyzer, MaskGenerator, RefineShadowMask, HeightPropagator,
        MetricConverter, ShadowDiffShader, CombineHeightmap;
    // INPUT is extracted from SAnalysis MutexQueue
    // Stage-0 satellite image texture - Wait from Python (INPUT)
    Texture* satellite_image = nullptr;
    // Stage-0.5 - Wait from Python (INPUT)
    Texture* building_mask = nullptr;
    Texture* terrain_heights = nullptr;
    // Stage-1 - Find optimal azimuth
    float azimuth;
    // Stage-2 - Generate shadow_mask AND norm_heights
    Texture* shadow_mask = nullptr;
    Texture* shadow_mask_ref = nullptr;
    Texture* norm_heights = nullptr;
    Texture* norm_heights_temp = nullptr;
    // Stage-3 - Pick elevation using binary search step
    //
    // Stage-4 - Generate unified heightmap (meters = terrain + building heights)
    Texture* u_heights = nullptr;
    // Stage-5 - Generate raymarched shadow mask using u_heightmap
    Texture* sim_shadow_mask = nullptr;
    Texture* diff_shadow_mask = nullptr;
    // Stage-6 - Calculate LOSS function for binary search criteria and move to Stage-3
    //
    // After some iterations, we OUTPUT u_heights

    // Final irradiance summation over time
    Texture* sum_irradiance = nullptr;

    Shader visualizeShader;

    GLuint ssboAngleScores = 0;
    const int NUM_DIRECTIONS = 64;

    GLuint ssboShadowDiff = 0;

    int FindOptimalAzimuth(float startAngle, float endAngle, float shadowThreshold);

public:

    void Debug_uHeights();

    SatelliteAnalyzer(int targetWidth = 5000, int targetHeight = 5000);

    float AngularAnalysis(float shadowThreshold);

    void PropagateHeight();

    void GenerateMasks(float optimal_azimuth, float shadowThreshold, int dist);
    void RefineShadow(float optimal_azimuth, int distanceThreshold);

    //void LoadUnifiedHeightmap(float default_building_height);

    void CombineMaskTerrainHeightmap(float elevation);

    float CalculateShadowDiff();


    struct SearchPoint {
        float azimuth;
        float elevation;
        float loss;
    };
    SearchPoint FindElevation(float optimal_azimuth);
    SearchPoint RefineAngles2D(float start_azimuth, float start_elevation);

    Texture* tmy_tex = nullptr;

    void AnalyzeFromQueue();

    void ResetAnalysisState() {
        AnalysisDone = false;
    }

    void InitTextures();
    void ResetTextures();
    void DeleteTextures();

    void ForcePipelineFlush();

    static void ResetGLContexts();
};