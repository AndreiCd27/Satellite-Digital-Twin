
#include "LightingModel.h"

const std::string path = "CRenderExtensions/Lighting";

SHLM::SHLM(Engine3D* engine, EngineConfig* config, AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax)
	: LightingService(engine, config), VoxelGrid(VoxelResolution, WorldMin, WorldMax),
	gridX((int)VoxelResolution.x), gridY((int)VoxelResolution.y), gridZ((int)VoxelResolution.z),
	worldMin(WorldMin), worldMax(WorldMax)
{

	SphericalHarmonics = new SH<SH_Order>();

	SH_Program.Setup((path+"/Shaders/SH.vert").c_str(), (path + "/Shaders/SH.frag").c_str());

}

void SHLM::BindToEngine(float FOVdeg, float zNear, float zFar) {

	std::function<void(float, float, float)> SH_f = [this](float f1, float f2, float f3) {
		this->SH_renderPass(f1, f2, f3);
	};

	config->RenderOverride = true;

	config->AddAction(RENDER_INSTANCES_STAGE, SH_f, FOVdeg, zNear, zFar);
}

// Called only by Load_Cubemap functions
void SHLM::GenTextures_Cubemap() {
	glGenTextures(4, texture3D_IDs);

	for (int i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_3D, texture3D_IDs[i]);

		// Texture Parameters
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
}

void SHLM::SH_Textures_Init() {

	for (int i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_3D, texture3D_IDs[i]);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, gridX, gridY, gridZ);
	}
}

void SHLM::GetMeshTrianglesAll(std::vector<GPU_Trig>& trigData) {
	//// Get Triangles From All Meshes

	auto indicies = engine->getScene()->GetEBO_Vector();
	auto vert = engine->getScene()->GetVBO_Vector();

	AVector3 tri[3];

	for (int i = 0; i < (int)indicies.size(); i++) {
		tri[i % 3] = vert[indicies[i]].POS;
		if (i % 3 == 2) {
			// Found triangle
			trigData.push_back(
				GPU_Trig(tri[0], tri[1], tri[2], (int)vert[indicies[i]].UV.UV)
			);
		}
	}
}

void SHLM::VoxelizeMeshesFullInside(Texture const* mipmap, Texture const* instances, ComputeShader& VoxelizeMeshCompute) {

	std::vector<GPU_Trig> trigData;
	GetMeshTrianglesAll(trigData);

	VoxelizeMeshCompute.SetDataSSBO<GPU_Trig>(trigData, (int)trigData.size(), idxTrigBuffer);

	VoxelizeMeshCompute.Activate();
	VoxelizeMeshCompute.SetInt("triangleCount", (int)trigData.size());
	VoxelizeMeshCompute.SetUniformVector3("worldMin", worldMin);
	VoxelizeMeshCompute.SetUniformVector3("worldMax", worldMax);
	VoxelizeMeshCompute.SetUniformVector3_int("gridRes", glm::ivec3(gridX, gridY, gridZ));
	VoxelizeMeshCompute.BindSSBO<0>(idxTrigBuffer);

	glBindImageTexture(0, mipmap->GetTexID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R16F);

	glBindImageTexture(1, instances->GetTexID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32I);


	int numGroupsX = (gridX + 15) / 16;
	int numGroupsY = 1;
	int numGroupsZ = (gridZ + 15) / 16;

	glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
	//glFinish();

}

void SHLM::MeshSphereDecomposition(Texture const* mipmap, Texture const* instances, ComputeShader& MeshDecomp, int LODmax) {

	//auto b_compute_t0 = std::chrono::high_resolution_clock::now();

	MeshDecomp.Activate();

	LODmax = 3;

	int numGroupsX = (gridX + 3) / 4;
	int numGroupsY = (gridY + 3) / 4;
	int numGroupsZ = (gridZ + 3) / 4;

	int maxSpheres = 100000;
	size_t bufferSize = sizeof(uint32_t) + (maxSpheres * sizeof(GPU_Blocker));

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, idxBlockerBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_COPY);

	uint32_t zeroCount = 0;
	glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, &zeroCount);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, idxBlockerBuffer);

	MeshDecomp.SetInt("LODmax", LODmax);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mipmap->GetTexID());

	MeshDecomp.SetInt("VoxelTex", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, instances->GetTexID());

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	MeshDecomp.SetInt("InstanceTex", 1);

	glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
	//glFinish();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, idxBlockerBuffer);
	uint32_t debugSphereCount = 0;
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &debugSphereCount);

	std::cout << ">>> DEBUG: Generated " << debugSphereCount << " sphere blockers in VRAM.\n";

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind

	//auto b_compute_t1 = std::chrono::high_resolution_clock::now();

	//std::chrono::duration<double, std::milli> duration = b_compute_t1 - b_compute_t0;

	//std::cout << "\n|_______ 3) MeshSphereDecomposition TIME: " << duration.count() << " ms _____________|\n\n";
}

void SHLM::ComputeCubemapVisibilitySH(Texture const* instances, ComputeShader& VoxelVisCompute) {

	//auto b_compute_t0 = std::chrono::high_resolution_clock::now();

	VoxelVisCompute.Activate();

	VoxelVisCompute.SetUniformVector3("worldMin", worldMin);
	VoxelVisCompute.SetUniformVector3("worldMax", worldMax);
	VoxelVisCompute.SetUniformVector3_int("gridRes", glm::ivec3(gridX, gridY, gridZ));
	// Bindings 0-3 are reserved for our textures
	VoxelVisCompute.BindSSBO<4>(idxBlockerBuffer);

	// Bind texture3Ds
	for (int i = 0; i < 4; i++) {
		glBindImageTexture(i, texture3D_IDs[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_3D, instances->GetTexID());
	VoxelVisCompute.SetInt("InstanceTex", 4);

	glDispatchCompute(gridX / 4, gridY / 4, gridZ / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	//glFinish();

	//auto b_compute_t1 = std::chrono::high_resolution_clock::now();

	//std::chrono::duration<double, std::milli> duration = b_compute_t1 - b_compute_t0;

	//std::cout << "\n|_______ 4) ComputeCubemapVisibilitySH TIME: " << duration.count() << " ms _____________|\n\n";
	
}

void SHLM::Load_Cubemap_GPU_ComputeShader_Extended() {

	//auto b_compute_t0 = std::chrono::high_resolution_clock::now();

	GenTextures_Cubemap();
	SH_Textures_Init();

	ComputeShader BakeVoxels;
	ComputeShader MeshFill;
	ComputeShader MeshDecomp;

	BakeVoxels.Setup((path + "/ComputeShaders/SH_VoxelVis.comp").c_str());
	MeshFill.Setup((path + "/ComputeShaders/FullVoxelFill.comp").c_str());
	MeshDecomp.Setup((path + "/ComputeShaders/MeshSphereDecomposition.comp").c_str());

	idxTrigBuffer = MeshFill.CreateSSBO();
	idxBlockerBuffer = MeshDecomp.CreateSSBO();

	Texture3D<GL_R16F, 1> VOXEL_MIPMAP(gridX, gridY, gridZ, false);

	Texture3D<GL_R32I, 1> VOXEL_INSTANCES(gridX, gridY, gridZ, false);

	std::vector<int> clearData(gridX * gridY * gridZ, -1);

	glBindTexture(GL_TEXTURE_3D, VOXEL_INSTANCES.GetTexID());
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, gridX, gridY, gridZ, GL_RED_INTEGER, GL_INT, clearData.data());
	glBindTexture(GL_TEXTURE_3D, 0);

	VoxelizeMeshesFullInside(&VOXEL_MIPMAP, &VOXEL_INSTANCES, MeshFill);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	//glFinish();

	VOXEL_MIPMAP.VerifyVoxelDataCPU(gridX, gridY, gridZ);

	VOXEL_MIPMAP.Make_Mipmap();

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	int maxLOD = VOXEL_MIPMAP.GetMaxLOD();
	//std::cout << " || MAX LEVEL OF DETAIL (LOD) = " << maxLOD << "\n";

	MeshSphereDecomposition(&VOXEL_MIPMAP, &VOXEL_INSTANCES, MeshDecomp, maxLOD);
	ComputeCubemapVisibilitySH(&VOXEL_INSTANCES, BakeVoxels);

	glDeleteBuffers(1, &idxBlockerBuffer);
	glDeleteBuffers(1, &idxTrigBuffer);

	//auto b_compute_t1 = std::chrono::high_resolution_clock::now();

	//std::chrono::duration<double, std::milli> duration = b_compute_t1 - b_compute_t0;

	//std::cout << "\n|_______ Load_Cubemap_GPU_ComputeShader TOTAL TIME: " << duration.count() << " ms _____________|\n\n";
}

SHLM::~SHLM() {
	delete SphericalHarmonics;
}


void SHLM::SH_renderPass(float FOVdeg, float zNear, float zFar) {

	SH_Program.Activate();
	engine->VAO_1.Bind();

	engine->registerCameraInput(FOVdeg, zNear, zFar);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[0]);
	glUniform1i(SH_Program.GetUniformLocation("V0"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[1]);
	glUniform1i(SH_Program.GetUniformLocation("V1"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[2]);
	glUniform1i(SH_Program.GetUniformLocation("V2"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_3D, texture3D_IDs[3]);
	glUniform1i(SH_Program.GetUniformLocation("V3"), 3);

	SH_Program.SetUniformVector3("WorldMin", VoxelGrid.GetWorldMin());
	SH_Program.SetUniformVector3("WorldMax", VoxelGrid.GetWorldMax());

	float LightSH[16]; for (int i = 0; i < 16; i++) LightSH[i] = 0.0f;
	AVector3 L = engine->SunCamera.Position.Normalize();
	// Evaluated direction in SH with the ZH convolution normalization constants
	//SphericalHarmonics->ComputeLogSHCoefficientsXYZR(L.x, L.y, L.z, SphericalHarmonics->ZHconv, LightSH);
	SphericalHarmonics->GetLightBasisYUp(L.x, L.y, L.z, LightSH);
	SphericalHarmonics->ApplyFunkHeckeCosine(LightSH, true);
	SH_Program.SetUniformVec4Array("LightSH", LightSH, 16);

	// Send the Light Normalized Direction for Lambertian shading
	SH_Program.SetUniformVector3("LightDir", L);

	//float ZHtoSH[4]; SphericalHarmonics->GetCombinedZHtoSH(ZHtoSH);
	//SH_Program.SetUniformVec4Array("ZHtoSH", ZHtoSH, 4);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, engine->window.getWidth(), engine->window.getHeight());

	// 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	engine->DrawAll();
}

