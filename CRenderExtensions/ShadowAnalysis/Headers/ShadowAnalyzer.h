#pragma once

enum class PIXEL_FORMAT {RGBA, RGB, RG, R};

#include "Engine3D.h"
#include "Bindings.h"
#include "Image.h"
#include <vector>
#include <utility>


class RawPixels : public Texture {
private:
    PIXEL_FORMAT m_format;

    // GL format mapping
    GLint GetGLInternalFormat(PIXEL_FORMAT format) const {
        switch (format) {
        case PIXEL_FORMAT::RGBA: return GL_RGBA8;
        case PIXEL_FORMAT::RGB:  return GL_RGB8;
        case PIXEL_FORMAT::RG:   return GL_RG8;
        case PIXEL_FORMAT::R:    return GL_R8;
        }
        return GL_R8;
    }

    GLenum GetGLFormat(PIXEL_FORMAT format) const {
        switch (format) {
        case PIXEL_FORMAT::RGBA: return GL_RGBA;
        case PIXEL_FORMAT::RGB:  return GL_RGB;
        case PIXEL_FORMAT::RG:   return GL_RG;
        case PIXEL_FORMAT::R:    return GL_RED;
        }
        return GL_RED;
    }

public:
    RawPixels(std::vector<unsigned char>& sourcePixels, PIXEL_FORMAT f, int w, int h)
        : m_format(f)
    {
        this->width = w;
        this->height = h;

        if (sourcePixels.empty()) return;

        SetupTexture(
            GetGLInternalFormat(m_format),
            GetGLFormat(m_format),
            GL_UNSIGNED_BYTE,
            GL_NEAREST, GL_NEAREST,
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
            sourcePixels.data()
        );

        // Clear pixels from RAM
        sourcePixels.clear();
        sourcePixels.shrink_to_fit();
    }

    RawPixels(PIXEL_FORMAT f, int w, int h)
        : m_format(f)
    {
        this->width = w;
        this->height = h;

        // Allocate Empty Texture
        SetupTexture(
            GetGLInternalFormat(m_format),
            GetGLFormat(m_format),
            GL_UNSIGNED_BYTE,
            GL_NEAREST, GL_NEAREST,
            GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
            nullptr
        );
    }

    ~RawPixels() {
        Delete();
    }

    // GPU objects are unique, we have to use move semantics
    RawPixels(RawPixels&& other) noexcept
        : m_format(other.m_format)
    {
        // Move attributes
        this->TexID = other.TexID;
        this->width = other.width;
        this->height = other.height;

        // Reset other texture state
        other.TexID = 0;
        other.width = 0;
        other.height = 0;
    }

    RawPixels& operator=(RawPixels&& other) noexcept {
        if (this != &other) {
            // Delete OpenGL resource before creating a new one
            Delete();

            this->m_format = other.m_format;
            this->TexID = other.TexID;
            this->width = other.width;
            this->height = other.height;

            other.TexID = 0;
            other.width = 0;
            other.height = 0;
        }
        return *this;
    }

    RawPixels(const RawPixels&) = delete;
    RawPixels& operator=(const RawPixels&) = delete;

    // Method for Compute Shader Image Binding
    void BindImage(unsigned int unit, unsigned int access) const {
        glBindImageTexture(unit, this->TexID, 0, GL_FALSE, 0, access, GetGLInternalFormat(m_format));
    }

    PIXEL_FORMAT GetPixelFormat() const { return m_format; }
};



class SAnalysis {
public:
    const RawPixels rawImage;
    const RawPixels maskImage;

    SAnalysis() : rawImage(PIXEL_FORMAT::R, 0, 0), maskImage(PIXEL_FORMAT::R, 0, 0) {}
	SAnalysis(std::vector<unsigned char>& pixels_0, PIXEL_FORMAT f_0, int w_0, int h_0,
		std::vector<unsigned char>& pixels_1, PIXEL_FORMAT f_1, int w_1, int h_1)
		: rawImage(pixels_0, f_0, w_0, h_0), maskImage(pixels_1, f_1, w_1, h_1) {
		// Creates an ShadowAnalysis on a raw image (given in pixels_0) and a mask image (given in pixels_1)
	}
    void BindRawImage(unsigned int unit, unsigned int access) const {
        rawImage.BindImage(unit, access);
    }
    void BindMaskImage(unsigned int unit, unsigned int access) const {
        maskImage.BindImage(unit, access);
    }
};

const std::string path = "CRenderExtensions/ShadowAnalysis";

struct AnalysisResult {
    std::unique_ptr<RawPixels> shadow_mask = nullptr;
    std::unique_ptr<RawPixels> heightmap_buildings = nullptr;
};

class ShadowAnalyzer {

    const float __PI = 3.14159265359f;

	MutexQueue<SAnalysis> m_SAnalysisQueue;

	ComputeShader DirectionalAnalyzer, MaskGenerator, HeightPropagator;
    Shader visualizeShader;

    unsigned int ssboAngleScores = 0;
    const int NUM_DIRECTIONS = 64;

    int FindOptimalAzimuth(const SAnalysis& s, float startAngle, float endAngle, float shadowThreshold) {

        DirectionalAnalyzer.ResetSSBO<unsigned int>(NUM_DIRECTIONS, ssboAngleScores);

        DirectionalAnalyzer.Activate();
        DirectionalAnalyzer.SetFloat("StartAngle", startAngle);
        DirectionalAnalyzer.SetFloat("EndAngle", endAngle);
        DirectionalAnalyzer.SetFloat("ShadowThreshold", shadowThreshold);
        DirectionalAnalyzer.SetFloat("DistanceThreshold", 20);

        int width = s.rawImage.GetWidth();
        int height = s.rawImage.GetHeight();
        if (width == 0 || height == 0) throw std::runtime_error("empty image");

        s.BindRawImage(0, GL_READ_ONLY);
        s.BindMaskImage(1, GL_READ_ONLY);

        DirectionalAnalyzer.BindSSBO<2>(ssboAngleScores);

        int groupX = (width + 15) / 16;
        int groupY = (height + 15) / 16;
        glDispatchCompute(groupX, groupY, 1);

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        std::vector<unsigned int> scores(NUM_DIRECTIONS);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAngleScores);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_DIRECTIONS * sizeof(unsigned int), scores.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        int bestIndex = 0;
        unsigned int maxScore = 0;
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            if (scores[i] > maxScore) {
                maxScore = scores[i];
                bestIndex = i;
            }
        }
        return bestIndex;
    }

public:

    ShadowAnalyzer() {
        std::cout << "Shadow Analyzer instantiated\n";
        DirectionalAnalyzer.Setup((path + "/ComputeShaders/azimuth.comp").c_str());
        MaskGenerator.Setup((path + "/ComputeShaders/gen_shadow_mask.comp").c_str());
        HeightPropagator.Setup((path + "/ComputeShaders/propagate_height.comp").c_str());
        if (ssboAngleScores == 0) {
            ssboAngleScores = DirectionalAnalyzer.CreateSSBO();
            DirectionalAnalyzer.AllocateEmptySSBO<unsigned int>(NUM_DIRECTIONS, ssboAngleScores);
        }
        visualizeShader.Setup((path + "/Shaders/render_analysis.vert").c_str(), (path + "/Shaders/render_analysis.frag").c_str());
    }

    float AngularAnalysis(const SAnalysis& s, float shadowThreshold) {

        float angleStep_0 = 2.0f * __PI / (float)NUM_DIRECTIONS;
        int optimal_idx_0 = FindOptimalAzimuth(s, 0.0f, 2.0f * __PI, shadowThreshold);
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

        int optimal_idx_1 = FindOptimalAzimuth(s, startArc, endArc, shadowThreshold);

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

    void PropagateHeight(const SAnalysis& s, const RawPixels& buildingHeightmap) {

        // Height Flood-Fill Pass
        HeightPropagator.Activate();
        int floodFillIterations = 15;

        for (int iter = 0; iter < floodFillIterations; iter++) {
            s.maskImage.BindImage(0, GL_READ_ONLY);

            glBindImageTexture(1, buildingHeightmap.GetTexID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);

            int w = buildingHeightmap.GetWidth();
            int h = buildingHeightmap.GetHeight();
            int groupX = (w + 15) / 16;
            int groupY = (h + 15) / 16;
            glDispatchCompute(groupX, groupY, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }

	void AnalyzeFromQueue(AnalysisResult& analysis) {
        std::unique_ptr<SAnalysis> sptr = m_SAnalysisQueue.TryPop();
        if (!sptr) return;

        // Find the general best orientation for the sun's azimuth angle
        float optimal_angle = AngularAnalysis(*sptr, 0.15f);

        // Image width, height
        int w = sptr->maskImage.GetWidth();
        int h = sptr->maskImage.GetHeight();

        // Create 2 new RawPixels objects to be populated via Compute Shader
        auto shadow_mask_ptr = std::make_unique<RawPixels>(PIXEL_FORMAT::R, w, h);
        auto heightmap_ptr = std::make_unique<RawPixels>(PIXEL_FORMAT::R, w, h);
        
        // Set Uniforms for Compute Shader
        MaskGenerator.Activate();
        MaskGenerator.SetFloat("ShadowAngle", optimal_angle);
        MaskGenerator.SetInt("DistanceThreshold", 40);
        MaskGenerator.SetFloat("ShadowThreshold", 0.15f);

        // Bind the 2 images (raw_image, mask_image(buildings)) AND
        // 2 generated images (shadow_mask_image, heightmap_image)
        sptr->BindRawImage(0, GL_READ_ONLY);
        sptr->BindMaskImage(1, GL_READ_ONLY);
        shadow_mask_ptr->BindImage(2, GL_WRITE_ONLY);
        heightmap_ptr->BindImage(3, GL_WRITE_ONLY);

        // Run Compute Shader
        int groupX = (w + 15) / 16;
        int groupY = (h + 15) / 16;
        glDispatchCompute(groupX, groupY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        PropagateHeight(*sptr, *heightmap_ptr);

        // Insert into analysis the resulting values from compute shader
        analysis.shadow_mask = std::move(shadow_mask_ptr);
        analysis.heightmap_buildings = std::move(heightmap_ptr);

        std::cout << "Shadow Mask & Height Mask generated; Analysis Complete!\n";

	}

    void PushInQueue(std::vector<unsigned char>& rawIMG_pixels, int rw, int rh, std::vector<unsigned char>& maskIMG_pixels, int mw, int mh) {
        auto sptr = std::make_unique<SAnalysis>(rawIMG_pixels, PIXEL_FORMAT::RGBA, rw, rh, maskIMG_pixels, PIXEL_FORMAT::RGBA, mw, mh);
        m_SAnalysisQueue.Push(std::move(sptr));
    }

    void VisualizeAnalysis(const AnalysisResult& a, Window* window) {
        visualizeShader.Activate();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, a.shadow_mask->GetTexID());
        visualizeShader.SetInt("u_shadowMaskTex", 0);

        const float quadVertices[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        VAO vao;
        vao.Setup();
        vao.Bind();

        VBO vbo;
        vbo.SetupFloatPTR(quadVertices, sizeof(quadVertices));

        vao.LinkVBO(vbo, 0, 2, GL_FLOAT, 4 * sizeof(float), GL_FALSE, (void*)0);

        vao.LinkVBO(vbo, 1, 2, GL_FLOAT, 4 * sizeof(float), GL_FALSE, (void*)(2 * sizeof(float)));

        glDrawArrays(GL_TRIANGLES, 0, 6);

        vao.Unbind();
        vbo.Delete();
        vao.Delete();

        glfwSwapBuffers(window->getWindow());

    }
    void SaveShadowMaskToDisk(const RawPixels& mask, const std::string& filename) {
        int w = mask.GetWidth();
        int h = mask.GetHeight();
        if (w <= 0 || h <= 0) return;

        std::vector<unsigned char> monoPixels(w * h, 0);
        glBindTexture(GL_TEXTURE_2D, mask.GetTexID());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, monoPixels.data());
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        std::vector<unsigned char> processedPixels(w * h, 255);

        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                int idx = y * w + x;

                if (monoPixels[idx] > 128) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int targetIdx = (y + dy) * w + (x + dx);
                            processedPixels[targetIdx] = 0;
                        }
                    }
                }
            }
        }

        stbi_write_png(filename.c_str(), w, h, 1, processedPixels.data(), w * 1);

        std::cout << "[DISK] Shadow mask from analysis saved successfully: " << filename << "\n";
    }
    void SaveHeightmapToDisk(const RawPixels& heightmap, const std::string& filename, float elevation) {
        int w = heightmap.GetWidth();
        int h = heightmap.GetHeight();
        if (w <= 0 || h <= 0) return;

        std::vector<unsigned char> rawHeights(w * h, 0);
        glBindTexture(GL_TEXTURE_2D, heightmap.GetTexID());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, rawHeights.data());
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        std::vector<unsigned char> metricHeights(w * h, 0);

        const float metersPerPixel = 0.5f;
        const int distanceThreshold = 40;

        const float grayscaleUnitsPerMeter = 10.0f;

        float maxDetectedMeters = 0.0f;

        for (int i = 0; i < w * h; i++) {
            if (rawHeights[i] > 0) {
                float shadowLengthInPixels = ((float)rawHeights[i] / 255.0f) * (float)distanceThreshold;

                float shadowLengthInMeters = shadowLengthInPixels * metersPerPixel;

                float heightInMeters = shadowLengthInMeters * std::tan(elevation);

                if (heightInMeters > maxDetectedMeters) {
                    maxDetectedMeters = heightInMeters;
                }

                // Grayscale
                float finalGrayscale = heightInMeters * grayscaleUnitsPerMeter;

                metricHeights[i] = (unsigned char)(finalGrayscale > 255.0f ? 255.0f : finalGrayscale);
            }
            else {
                metricHeights[i] = 0;
            }
        }

        std::cout << "[METRIC] Highest building: " << maxDetectedMeters << " meters\n";

        stbi_write_png(filename.c_str(), w, h, 1, metricHeights.data(), w * 1);

        std::cout << "[DISK] Building Heightmap from analysis saved successfully: " << filename << "\n";
    }

};