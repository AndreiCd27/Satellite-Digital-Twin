
#include "ShadowAnalyzer.h"


SatelliteAnalyzer::SatelliteAnalyzer(int targetWidth, int targetHeight) : targetWidth(targetWidth), targetHeight(targetHeight) {
    //std::cout << "Satellite Analyzer instantiated\n";

    DirectionalAnalyzer.Setup((path + "/ComputeShaders/azimuth.comp").c_str(), "Directional Analyzer");
    //MaskGenerator.Setup((path + "/ComputeShaders/gen_shadow_mask.comp").c_str(), "Mask Generator");
    MaskGenerator.Setup((path + "/ComputeShaders/shadow_mask_adv.comp").c_str(), "Mask Generator");
    RefineShadowMask.Setup((path + "/ComputeShaders/refine_shadow_mask.comp").c_str(), "Refine Shadow Mask");
    HeightPropagator.Setup((path + "/ComputeShaders/propagate_height.comp").c_str(), "Height Propagator");
    MetricConverter.Setup((path + "/ComputeShaders/convert_metric_heightmap.comp").c_str(), "Metric Converter");
    ShadowDiffShader.Setup((path + "/ComputeShaders/compute_shadow_diff.comp").c_str(), "Shadow Diff Compute");
    CombineHeightmap.Setup((path + "/ComputeShaders/combine_heightmap.comp").c_str(), "Combine Heightmap");

    InitTextures();

    // Azimuth SSBO
    ssboAngleScores = DirectionalAnalyzer.CreateSSBO();
    DirectionalAnalyzer.AllocateEmptySSBO<unsigned int>(NUM_DIRECTIONS, ssboAngleScores);

    // SSBO Shadow Loss/Diff
    glGenBuffers(1, &ssboShadowDiff);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboShadowDiff);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    visualizeShader.Setup((path + "/Shaders/render_analysis.vert").c_str(), (path + "/Shaders/render_analysis.frag").c_str());
}

void SatelliteAnalyzer::InitTextures() {
    // Fixed texture dimensiosn
    // Shadow Mask
    shadow_mask = CommandBuffer::InitTexSlot("shadow_mask").get();
    shadow_mask->GenTex2D(targetWidth, targetHeight);
    shadow_mask->MinMagFilter(GL_LINEAR, GL_LINEAR);
    shadow_mask->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    shadow_mask->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Shadow Mask (Refined)
    shadow_mask_ref = CommandBuffer::InitTexSlot("shadow_mask_ref").get();
    shadow_mask_ref->GenTex2D(targetWidth, targetHeight);
    shadow_mask_ref->MinMagFilter(GL_LINEAR, GL_LINEAR);
    shadow_mask_ref->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    shadow_mask_ref->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Normalized heightmap
    norm_heights = CommandBuffer::InitTexSlot("norm_heights").get();
    norm_heights->GenTex2D(targetWidth, targetHeight);
    norm_heights->MinMagFilter(GL_LINEAR, GL_LINEAR);
    norm_heights->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    norm_heights->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    norm_heights_temp = CommandBuffer::InitTexSlot("norm_heights_temp").get();
    norm_heights_temp->GenTex2D(targetWidth, targetHeight);
    norm_heights_temp->MinMagFilter(GL_LINEAR, GL_LINEAR);
    norm_heights_temp->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    norm_heights_temp->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Unified heightmap
    u_heights = CommandBuffer::InitTexSlot("u_heights").get();
    u_heights->GenTex2D(targetWidth, targetHeight);
    u_heights->MinMagFilter(GL_NEAREST, GL_NEAREST);
    u_heights->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    u_heights->CreateTex2D(GL_RG32F, GL_RG, GL_FLOAT, nullptr);

    // Raymarched Shadow Mask
    sim_shadow_mask = CommandBuffer::InitTexSlot("sim_shadow_mask").get();
    sim_shadow_mask->GenTex2D(targetWidth, targetHeight);
    sim_shadow_mask->MinMagFilter(GL_LINEAR, GL_LINEAR);
    sim_shadow_mask->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    sim_shadow_mask->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Combined raymarched and real shadow masks
    diff_shadow_mask = CommandBuffer::InitTexSlot("diff_shadow_mask").get();
    diff_shadow_mask->GenTex2D(targetWidth, targetHeight);
    diff_shadow_mask->MinMagFilter(GL_LINEAR, GL_LINEAR);
    diff_shadow_mask->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    diff_shadow_mask->CreateTex2D(GL_RG8, GL_RG, GL_UNSIGNED_BYTE, nullptr);
}

int SatelliteAnalyzer::FindOptimalAzimuth(float startAngle, float endAngle, 
    float shadowThreshold) {

    DirectionalAnalyzer.ClearSSBO<unsigned int>(NUM_DIRECTIONS, ssboAngleScores);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    DirectionalAnalyzer.Activate();
    DirectionalAnalyzer.SetFloat("StartAngle", startAngle);
    DirectionalAnalyzer.SetFloat("EndAngle", endAngle);
    DirectionalAnalyzer.SetFloat("ShadowThreshold", shadowThreshold);
    DirectionalAnalyzer.SetInt("DistanceThreshold", 20);

    int width = satellite_image->GetWidth();
    int height = satellite_image->GetHeight();
    if (width == 0 || height == 0) throw std::runtime_error("empty image");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, satellite_image->GetTexID());
    DirectionalAnalyzer.SetInt("rawTex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, building_mask->GetTexID());
    DirectionalAnalyzer.SetInt("maskTex", 1);

    DirectionalAnalyzer.BindSSBO<2>(ssboAngleScores);

    int groupX = (width + 15) / 16;
    int groupY = (height + 15) / 16;
    glDispatchCompute(groupX, groupY, 1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    // UNBIND
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindImageTexture(6, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glBindImageTexture(7, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "[OPENGL ERROR] [AZIMUTH COMPUTE] Error after Compute Dispatch: 0x" 
            << std::hex << err << " " << std::dec << 0 << "\n";
    }

    std::vector<unsigned int> scores(NUM_DIRECTIONS);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAngleScores);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_DIRECTIONS * sizeof(unsigned int), scores.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    int bestIndex = 0;
    unsigned int maxScore = 0;
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        //std::cout << "Score " << i << ": " << scores[i] << "---------------------------\n";
        if (scores[i] > maxScore) {
            maxScore = scores[i];
            bestIndex = i;
        }
    }
    return bestIndex;
}

float SatelliteAnalyzer::AngularAnalysis(float shadowThreshold) {

    float angleStep_0 = 2.0f * __PI / (float)NUM_DIRECTIONS;
    int optimal_idx_0 = FindOptimalAzimuth(0.0f, 2.0f * __PI, shadowThreshold);
    float optimal_angle_0 = (float)optimal_idx_0 * angleStep_0;

    std::cout << "Optimal Azimuth 0: " << optimal_angle_0 << " RADIANS\n";

    float windowRadius = angleStep_0 * 2.0f;
    float startArc = optimal_angle_0 - windowRadius;
    float endArc = optimal_angle_0 + windowRadius;

    // shader corection, no negative interval
    if (startArc < 0.0f) {
        startArc += 2.0f * __PI;
        endArc += 2.0f * __PI;
    }

    int optimal_idx_1 = FindOptimalAzimuth(startArc, endArc, shadowThreshold);

    // Linear inrerpolation between end and start arc boundaries
    float progress = (float)optimal_idx_1 / (float)(NUM_DIRECTIONS - 1);
    float optimal_angle_1 = startArc + (progress * (endArc - startArc));

    // Go back to standard angle interval if angle overflowed beyond 2pi
    if (optimal_angle_1 > 2.0f * __PI) {
        optimal_angle_1 -= 2.0f * __PI;
    }

    std::cout << "Optimal Azimuth 1: " << optimal_angle_1 << " RADIANS\n";

    return optimal_angle_1;
}
void SatelliteAnalyzer::PropagateHeight() {
    HeightPropagator.Activate();
    int floodFillIterations = 128;

    // Texture Pointers for source -> destination sampling
    Texture* srcHeightTex = norm_heights;
    Texture* destHeightTex = norm_heights_temp;

    int w = norm_heights->GetWidth();
    int h = norm_heights->GetHeight();
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    for (int iter = 0; iter < floodFillIterations; iter++) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, building_mask->GetTexID());
        HeightPropagator.SetInt("maskTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, srcHeightTex->GetTexID());
        HeightPropagator.SetInt("srcHeightMapTex", 1);

        destHeightTex->BindImage(2, GL_WRITE_ONLY);

        glDispatchCompute(groupX, groupY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // Swap pointers for next iter
        std::swap(srcHeightTex, destHeightTex);
    }

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    // Copy texture
    if (srcHeightTex != norm_heights) {
        glCopyImageSubData(
            srcHeightTex->GetTexID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            norm_heights->GetTexID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            w, h, 1
        );
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Unbind textures
    glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

}

void SatelliteAnalyzer::GenerateMasks(float optimal_azimuth, float shadowThreshold, int dist) {

    // Set Uniforms for Compute Shader
    MaskGenerator.Activate();
    MaskGenerator.SetFloat("ShadowAngle", optimal_azimuth);
    MaskGenerator.SetInt("DistanceThreshold", dist);
    MaskGenerator.SetFloat("ShadowThreshold", shadowThreshold);

    // Image width, height for building_mask
    int w = building_mask->GetWidth();
    int h = building_mask->GetHeight();
    // Groups for Compute Shader
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    // Bind the 2 images (raw_image, mask_image(buildings)) AND
    // 2 generated images (shadow_mask_image, heightmap_image)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, satellite_image->GetTexID());
    MaskGenerator.SetInt("rawTex", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, building_mask->GetTexID());
    MaskGenerator.SetInt("maskTex", 1);

    shadow_mask->BindImage(2, GL_WRITE_ONLY);
    norm_heights->BindImage(3, GL_WRITE_ONLY);

    // Run Compute Shader
    glDispatchCompute(groupX, groupY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind textures shadow_mask and norm_heights
    glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindImageTexture(3, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

    // Unbind texture slots used by satellite_image and building_mask
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "[OPENGL ERROR] [SHADOW MASK] Error after Compute Dispatch: 0x" 
            << std::hex << err << " " << std::dec << 0 << "\n";
    }

    RefineShadow(optimal_azimuth, dist);
}

void SatelliteAnalyzer::RefineShadow(float optimal_azimuth, int distanceThreshold) {

    RefineShadowMask.Activate();

    RefineShadowMask.SetUniformVector2("sunDir", cos(optimal_azimuth), sin(optimal_azimuth));
    RefineShadowMask.SetInt("DistanceThreshold", distanceThreshold);

    int w = shadow_mask->GetWidth();
    int h = shadow_mask->GetHeight();
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    building_mask->BindImage(0, GL_READ_ONLY);
    shadow_mask->BindImage(1, GL_READ_ONLY);
    shadow_mask_ref->BindImage(2, GL_WRITE_ONLY);
    glDispatchCompute(groupX, groupY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "[OPENGL ERROR] [REFINEMENT] Error after Compute Dispatch: 0x"
            << std::hex << err << std::dec << "\n";
    }
}


float SatelliteAnalyzer::CalculateShadowDiff() {

    GLuint zero[2] = {0,0};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboShadowDiff);
    // Reset SSBO value to 0
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero), &zero);

    ShadowDiffShader.Activate();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_mask_ref->GetTexID());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sim_shadow_mask->GetTexID());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboShadowDiff); // binding 2: DiffBuffer

    diff_shadow_mask->BindImage(3, GL_WRITE_ONLY);

    // Launch threads on the GPU
    int groupX = (shadow_mask_ref->GetWidth() + 15) / 16;
    int groupY = (shadow_mask_ref->GetHeight() + 15) / 16;
    glDispatchCompute(groupX, groupY, 1);

    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "[OPENGL ERROR] [SHADOW LOSS] Error after Compute Dispatch: 0x"
            << std::hex << err << " " << std::dec << 0 << "\n";
    }

    GLuint IoU_data[2] = { 0, 0 };
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IoU_data), IoU_data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Compute IoU
    float interCnt = (float)IoU_data[0];
    float unionCnt = (float)IoU_data[1];

    float loss = 1.0f;
    if (unionCnt > 0.0f) {
        float iou = interCnt / unionCnt;
        loss = 1.0f - iou;
    }

    /* glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glBindImageTexture(3, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG); */
    /*
    std::cout << "[SHADOW_LOSS] Intersection pixels: " << IoU_data[0] << " | Union pixels: " << IoU_data[1] 
        << " | IoU Loss = " << loss << "\n";
        */
    return loss;
}

SatelliteAnalyzer::SearchPoint SatelliteAnalyzer::FindElevation(float optimal_azimuth) {
    float best_elevation = 0.0f;
    float min_loss = 1.0f;

    int w = shadow_mask_ref->GetWidth();
    int h = shadow_mask_ref->GetHeight();
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    const int STEPS = 16;
    float min_angle = 10.0f * (3.14159265f / 180.0f);
    float max_angle = 80.0f * (3.14159265f / 180.0f);

    for (int step = 0; step < STEPS; step++) {
        float test_elevation = min_angle + (float)step / (STEPS - 1) * (max_angle - min_angle);

        //std::cout << "[SATELLITE_ANALYSIS] Testing elevation: " << test_elevation << " radians (" << test_elevation * (180.0f / 3.14159f) << " deg)\n";

        RayMarchOperation.RaymarchUnifiedScene(optimal_azimuth, test_elevation,
            u_heights, sim_shadow_mask, groupX, groupY);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        float current_loss = CalculateShadowDiff();
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        //std::cout << "[SHADOW LOSS] IoU Loss = " << current_loss << "\n";

        if (current_loss < min_loss) {
            min_loss = current_loss;
            best_elevation = test_elevation;
        }
    }

    SearchPoint s; s.elevation = best_elevation; s.azimuth = optimal_azimuth; s.loss = min_loss;
    return s;
}



void SatelliteAnalyzer::CombineMaskTerrainHeightmap(float elevation) {

    MetricConverter.Activate();
    MetricConverter.SetFloat("ElevationAngle", elevation);
    MetricConverter.SetFloat("MetersPerPixel", 0.3f);
    MetricConverter.SetInt("DistanceThreshold", 40);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, norm_heights->GetTexID());

    glBindImageTexture(1, u_heights->GetTexID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, terrain_heights->GetTexID());

    // Launch convert_metric_heightmap.comp
    // Image width, height
    int w = building_mask->GetWidth();
    int h = building_mask->GetHeight();
    // Groups for Compute Shader
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    glDispatchCompute(groupX, groupY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "[OPENGL ERROR] [METRIC HEIGHTMAP] Error after Compute Dispatch: 0x"
            << std::hex << err << " " << std::dec << 0 << "\n";
    }

    /* glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F); */
}

SatelliteAnalyzer::SearchPoint SatelliteAnalyzer::RefineAngles2D(float start_azimuth, float start_elevation) {
    int w = shadow_mask_ref->GetWidth();
    int h = shadow_mask_ref->GetHeight();
    int groupX = (w + 15) / 16;
    int groupY = (h + 15) / 16;

    // Evaluate initial start angles
    RayMarchOperation.RaymarchUnifiedScene(start_azimuth, start_elevation, u_heights, sim_shadow_mask, groupX, groupY);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    float start_loss = CalculateShadowDiff();

    SearchPoint best = { start_azimuth, start_elevation, start_loss };

    float step_azimuth = 2.0f * (3.14159265f / 180.0f);
    float step_elevation = 2.0f * (3.14159265f / 180.0f);

    float min_step = 0.05f * (3.14159265f / 180.0f);

    int iterations = 0;
    const int MAX_ITERATIONS = 60;

    while ((step_azimuth > min_step || step_elevation > min_step) && iterations < MAX_ITERATIONS) {
        iterations++;
        bool improved = false;

        // Test out 8 directions
        SearchPoint neighbors[8] = {
            { best.azimuth + step_azimuth, best.elevation, 1.0f },
            { best.azimuth - step_azimuth, best.elevation, 1.0f },
            { best.azimuth, best.elevation + step_elevation, 1.0f },
            { best.azimuth, best.elevation - step_elevation, 1.0f },
            { best.azimuth + step_azimuth, best.elevation + step_elevation, 1.0f },
            { best.azimuth + step_azimuth, best.elevation - step_elevation, 1.0f },
            { best.azimuth - step_azimuth, best.elevation + step_elevation, 1.0f },
            { best.azimuth - step_azimuth, best.elevation - step_elevation, 1.0f }
        };

        for (int i = 0; i < 8; i++) {
            // Plajă de siguranță pentru elevație
            if (neighbors[i].elevation < 0.15f || neighbors[i].elevation > 1.3f) continue;

            RayMarchOperation.RaymarchUnifiedScene(neighbors[i].azimuth, neighbors[i].elevation,
                u_heights, sim_shadow_mask, groupX, groupY);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            neighbors[i].loss = CalculateShadowDiff();

            if (neighbors[i].loss < best.loss) {
                best = neighbors[i];
                improved = true;
            }
        }

        if (!improved) {
            // Angle Steps too BIG, decrease slowly to reach a local minima
            step_azimuth *= 0.75f;
            step_elevation *= 0.75f;
        }
    }

    std::cout << "[REFINEMENT] Finished in " << iterations << " steps. Final IoU Loss: " << best.loss << "\n";
    return best;
}

void SatelliteAnalyzer::Debug_uHeights() {

    int w = u_heights->GetWidth();
    int h = u_heights->GetHeight();

    std::vector<float> cpuPixels(w * h * 2, 0.0f);

    glBindTexture(GL_TEXTURE_2D, u_heights->GetTexID());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, cpuPixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    float avgH = 0.0f;
    float maxH = 0.0f;
    int bcnt = 0;

    for (int i = 0; i < w * h * 2; i += 2) {
        if (cpuPixels[i] > 0.0f) {
            avgH += cpuPixels[i];
            bcnt++;
        }
        if (cpuPixels[i] > maxH) {
            maxH = cpuPixels[i];
        }
    }

    std::cout << "--------------------------------------------------------------\n";
    std::cout << "[GPU HEIGHT TEXTURE CHECK] Building Pixels Count: " << bcnt << "\n";
    std::cout << "[GPU HEIGTH TEXTURE CHECK] Max Building Height: " << maxH << "\n";
    std::cout << "[GPU HEIGHT TEXTURE CHECK] Average Building Height: " << avgH / (float(bcnt) + 0.000001f) << "\n";
    std::cout << "--------------------------------------------------------------\n";
}

void SatelliteAnalyzer::AnalyzeFromQueue() {
    if (AnalysisDone) return;

    glFinish();
    ForcePipelineFlush();
    ResetGLContexts();
    ResetTextures();

    std::cout << "[SATELLITE_ANALYSIS] Image Analysis started! \n";

    auto bmask_ptr = CommandBuffer::GetTexSlot("buildings_mask");
    if (bmask_ptr == nullptr) return;
    auto terrain_ptr = CommandBuffer::GetTexSlot("heightmap");
    if (terrain_ptr == nullptr) return;
    auto satellite_ptr = CommandBuffer::GetTexSlot("satellite");
    if (satellite_ptr == nullptr) return;

    std::cout << "[SATELLITE_ANALYSIS] Found building_mask, terrain_heightmap and satellite_image! \n";

    building_mask = bmask_ptr.get();
    terrain_heights = terrain_ptr.get();
    satellite_image = satellite_ptr.get();

    // Find the general best orientation for the sun's azimuth angle
    float optimal_azimuth = AngularAnalysis(0.25f);

    std::cout << "[SATELLITE_ANALYSIS] Found optimal sun azimuth: " << optimal_azimuth << " radians\n";

    float optimal_elevation = 0.785f;
    float min_IoU_loss = 1.0f;

    std::pair<float,int> params[] = {{0.25f, 120}, {0.275f, 120}, {0.3f, 120}};

    for (auto& param : params) {
        GenerateMasks(optimal_azimuth, param.first, param.second);

        // Force GPU to finish writing masks before PropagateHeight reads them
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        PropagateHeight();

        // Force GPU to finish height propagation before combining textures
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        CombineMaskTerrainHeightmap(0.785f);

        // Force GPU to finish combining before FindElevation reads the final texture
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        auto s = FindElevation(optimal_azimuth);

        if (s.loss < min_IoU_loss) {
            min_IoU_loss = s.loss;
            optimal_elevation = s.elevation;
            std::cout << "[SATELLITE_ANALYSIS] Found optimal sun elevation: " << s.elevation << " with loss: " << s.loss << "\n";
        }
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    CombineMaskTerrainHeightmap(optimal_elevation);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
    Debug_uHeights();

    for (int i = 0; i < 8; ++i) {
        glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::cout << "[SATELLITE_ANALYSIS] Generated metric heightmap, analysis complete!\n";

    AnalysisDone = true;
}


void SatelliteAnalyzer::ResetTextures() {

    if (shadow_mask) shadow_mask->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    if (shadow_mask_ref) shadow_mask_ref->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    if (norm_heights) norm_heights->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    if (norm_heights_temp) norm_heights_temp->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    if (u_heights) u_heights->CreateTex2D(GL_RG32F, GL_RG, GL_FLOAT, nullptr);
    if (sim_shadow_mask) sim_shadow_mask->CreateTex2D(GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    if (diff_shadow_mask) diff_shadow_mask->CreateTex2D(GL_RG8, GL_RG, GL_UNSIGNED_BYTE, nullptr);

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void SatelliteAnalyzer::ResetGLContexts() {
    // Reset OpenGL texture & SSBO contexts
    for (int i = 0; i < 16; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    for (int i = 0; i < 8; ++i) {
        glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);
}

void SatelliteAnalyzer::ForcePipelineFlush() {

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
        GL_TEXTURE_FETCH_BARRIER_BIT |
        GL_TEXTURE_UPDATE_BARRIER_BIT |
        GL_BUFFER_UPDATE_BARRIER_BIT);

    for (int i = 0; i < 8; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);

    for (int i = 0; i < 8; ++i) {
        glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUseProgram(0);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}


void SatelliteAnalyzer::DeleteTextures() {
    if (shadow_mask) shadow_mask->Delete();
    if (shadow_mask_ref) shadow_mask_ref->Delete();
    if (norm_heights) norm_heights->Delete();
    if (norm_heights_temp) norm_heights_temp->Delete();
    if (u_heights) u_heights->Delete();
    if (sim_shadow_mask) sim_shadow_mask->Delete();
    if (diff_shadow_mask) diff_shadow_mask->Delete();
}
