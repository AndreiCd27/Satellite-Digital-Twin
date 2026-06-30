#pragma once


#include "precompile.h"
#include "shaderClass.h"
#include "ComputeShader.h"
#include "SH.h"
#include "SH_VoxelGrid.h"

#include <chrono>

#include "Engine3D.h"

// LightingService

class LightingService {
protected:
	Engine3D* engine = nullptr;
	EngineConfig* config = nullptr;
public:
	LightingService(Engine3D* engine, EngineConfig* config) : engine(engine), config(config) {}
};

constexpr int SH_Order = 3;
constexpr int SphereDirCount = 64;

// Spherical Harmonic Lighting Model (SHLM)

struct LightParameters {
	// Light parameters
	float DirectShadowStrength = 0.85;
	float AmbientShadowContrast = 1.0;
	float AmbientShadowIntensity = 0.7;
	float DirectLightIntensity = 1.0; // Exposure parameter for sun light
	float AmbientLightIntensity = 0.3; // Exposure parameter for sky/indirect light

	void Set(float DSS, float ASC, float ASI, float DLI, float ALI) {
		DirectShadowStrength = DSS;
		AmbientShadowContrast = ASC;
		AmbientShadowIntensity = ASI;
		DirectLightIntensity = DLI;
		AmbientLightIntensity = ALI;
	}
};

class SHLM : public LightingService {
	// For shadow rendering using Spherical Harmonics (see SHEXP.h)

	SH<SH_Order>* SphericalHarmonics = nullptr;

	SHVoxelGrid<SH_Order> VoxelGrid;

	Shader SH_Program;

	Shader shadowMaskProgram;

	GLuint texture3D_IDs[4];
	std::shared_ptr<Texture> vis_tex[4];

	//ComputeShader VoxelizeMeshes, BakeVoxels;
	GLuint idxTrigBuffer, idxBlockerBuffer;

	void GenTextures_Cubemap();

	//void ComputeShadersInit(const std::string& shdrName0, const std::string& shdrName1);
	void SH_Textures_Init();
	void SH_Textures_Init_2D();

	GLenum TEXTURE_DIMENSIONALITY = GL_TEXTURE_2D;
	Texture* UnifiedHeightmap = nullptr;
	Texture* SatelliteTex = nullptr;

	Texture* sum_irradiance_in = nullptr;
	Texture* sum_irradiance_out = nullptr;

	ComputeShader BakeHeightmap;

	ComputeShader SumIrradiance;

	// Helpers for SH_VoxelVis.comp setup
	// Stage 1: Voxelize mesh surfaces
	// - Run VoxelizeMeshes3.comp
	void VoxelizeMeshesFullInside(Texture const* mipmap, Texture const* instances, ComputeShader& VoxelizeMeshCompute);
	// Stage 2: Voxelized Mesh sphere decomposition
	// - Run MeshSphereDecomposition.comp
	void MeshSphereDecomposition(Texture const* mipmap, Texture const* instances, ComputeShader& MeshDecomp, int LODmax);
	// Stage 3: Compute Visibility Functions for each Voxel into SH-basis
	// - Run SH_VoxelVis.comp
	void ComputeCubemapVisibilitySH(Texture const* instances, ComputeShader& VoxelVisCompute);

	// Easy access to these variables
	const int gridX, gridY, gridZ;
	const AVector3 worldMin, worldMax;

public:

	inline static LightParameters LightParams;

	void SetUnifiedHeightmap(Texture* u_heightmap);
	void SetSatelliteTexture(Texture* satelliteTex);

	static int SkipN_Days;

	struct GPU_Blocker {
		glm::vec4 pos_rad;
		glm::ivec4 instanceID;
	};

	struct GPU_Trig {
		glm::vec4 v0, v1, v2;
		GPU_Trig(const AVector3& V0, const AVector3& V1, const AVector3& V2, const int InsID)
			: v0(V0.x, V0.y, V0.z, (float)InsID), v1(V1.x, V1.y, V1.z, 0.0f), v2(V2.x, V2.y, V2.z, 0.0f)
		{
		}
	};

	void GetMeshTrianglesAll(std::vector<GPU_Trig>& trigData);

	// SPHERICAL HARMONICS RENDERING //////////////////////////////////////////////////

	SHLM(Engine3D* engine, EngineConfig* config, AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax);

	void BindToEngine(float FOVdeg, float zNear, float zFar);

	//
	void Load_Cubemap_GPU_ComputeShader_Extended();
	void SH_Heightmap_Shading_Compute();

	GLuint HeightmapTexID = 0;
	GLuint BuildingsMaskTexID = 0;
	AVector3 SphereDirs[SphereDirCount];

	////////////////////////////////////////////////////////////////////////////////////

	void SH_renderPass(float FOVdeg, float zNear, float zFar);

	~SHLM();

	void ElevateVBO();
	void SumIrradianceOverTime();

	void ResetPipelineForNextImage();
	void ProcessEntireYear(int total_maxt);

};