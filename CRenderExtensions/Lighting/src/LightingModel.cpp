
#include "LightingModel.h"

const std::string path = "CRenderExtensions/Lighting";

SHLM::SHLM(Engine3D* engine, EngineConfig* config, AVector3 VoxelResolution, AVector3 WorldMin, AVector3 WorldMax)
	: LightingService(engine, config), VoxelGrid(VoxelResolution, WorldMin, WorldMax),
	gridX((int)VoxelResolution.x), gridY((int)VoxelResolution.y), gridZ((int)VoxelResolution.z),
	worldMin(WorldMin), worldMax(WorldMax)
{

	SphericalHarmonics = new SH<SH_Order>();

	if (TEXTURE_DIMENSIONALITY == GL_TEXTURE_3D) {
		SH_Program.Setup((path + "/Shaders/SH.vert").c_str(), (path + "/Shaders/SH.frag").c_str());
	}
	else {
		SH_Program.Setup((path + "/Shaders/SH.vert").c_str(), (path + "/Shaders/SH_2D.frag").c_str());
	}

	BakeHeightmap.Setup((path + "/ComputeShaders/SH_Heightmap.comp").c_str(), "SH Heightmap Compute Shader");

	SumIrradiance.Setup((path + "/ComputeShaders/AVG_Irradiance.comp").c_str(), "Irradiance Sumation");

	// Summation of irradiance values for input Watt/m^2
	sum_irradiance_in = CommandBuffer::InitTexSlot("sum_irradiance_in").get();
	sum_irradiance_in->GenTex2D(5000, 5000);
	sum_irradiance_in->MinMagFilter(GL_LINEAR, GL_LINEAR);
	sum_irradiance_in->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	sum_irradiance_in->CreateStorageTex2D(5000, 5000, GL_R32F);

	// Summation of irradiance values for output Watt/m^2
	sum_irradiance_out = CommandBuffer::InitTexSlot("sum_irradiance_out").get();
	sum_irradiance_out->GenTex2D(5000, 5000);
	sum_irradiance_out->MinMagFilter(GL_LINEAR, GL_LINEAR);
	sum_irradiance_out->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	sum_irradiance_out->CreateStorageTex2D(5000, 5000, GL_R32F);
	
	// First clear all pixels to black
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sum_irradiance_in->GetTexID(), 0);
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glClearBufferfv(GL_COLOR, 0, clearColor);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
	// For out image too
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sum_irradiance_out->GetTexID(), 0);
	glClearBufferfv(GL_COLOR, 0, clearColor);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);


	for (int i = 0; i < SphereDirCount; i++) {
		float phi = __PI * (sqrt(5.0) - 1.0);

		float y = float(i) / float(SphereDirCount);
		float radius = sqrt(1.0 - y * y);

		float theta = phi * float(i);

		float x = cos(theta) * radius;
		float z = sin(theta) * radius;

		SphereDirs[i] = AVector3(x, y, z);
	}
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
		// Changed to GL_RGBA16F for less VRAM consumption
		// Precision is kept
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, gridX, gridY, gridZ);
	}
}
void SHLM::SH_Textures_Init_2D() {

	for (int i = 0; i < 4; i++) {
		vis_tex[i] = CommandBuffer::InitTexSlot("Vis" + std::to_string(i));
		vis_tex[i]->SetWidth(gridX);
		vis_tex[i]->SetHeight(gridZ);

		vis_tex[i]->SetupTexture(GL_RGBA16F,GL_RGBA,GL_FLOAT,GL_LINEAR,GL_LINEAR,GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE,nullptr);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
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
	int triCount = (int)trigData.size();
	// Clear resources immediately
	trigData.clear();
	trigData.shrink_to_fit();

	VoxelizeMeshCompute.Activate();
	VoxelizeMeshCompute.SetInt("triangleCount", triCount);
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
		glBindImageTexture(i, texture3D_IDs[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
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
	static bool init_called = false;
	//auto b_compute_t0 = std::chrono::high_resolution_clock::now();
	if (!init_called) {
		GenTextures_Cubemap();
		SH_Textures_Init();
		init_called = true;
	}

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

void SHLM::SH_Heightmap_Shading_Compute() {

	static bool init_called = false;
	//auto b_compute_t0 = std::chrono::high_resolution_clock::now();
	if (!init_called) {
		SH_Textures_Init_2D();
		init_called = true;
	}

	// building mask and heightmap textures are given after Python work load finishes

	BakeHeightmap.Activate();

	BakeHeightmap.SetUniformVector3("worldMin", worldMin);
	BakeHeightmap.SetUniformVector3("worldMax", worldMax);
	BakeHeightmap.SetUniformVector3_int("gridRes", glm::ivec3(gridX, gridY, gridZ));
	//BakeHeightmap.SetUniformVec3Array("SphereDirs", reinterpret_cast<float*>(&SphereDirs), SphereDirCount);

	// Bind texture2Ds
	for (int i = 0; i < 4; i++) {
		vis_tex[i]->BindImage(i, GL_WRITE_ONLY);
	}

	if (TEXTURE_DIMENSIONALITY == GL_TEXTURE_2D && UnifiedHeightmap != nullptr) {
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(TEXTURE_DIMENSIONALITY, UnifiedHeightmap->GetTexID());
		BakeHeightmap.SetInt("uHeightmap",4);
	}

	int numGroupsX = (gridX + 15) / 16;
	int numGroupsZ = (gridZ + 15) / 16;
	glDispatchCompute(numGroupsX, numGroupsZ, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	for (int i = 0; i < 4; i++) {
		glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
	}
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(TEXTURE_DIMENSIONALITY, 0);

	bool glerr = false;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cout << "[OPENGL ERROR] Error after Compute Dispatch: 0x" << std::hex << err <<" "<< std::dec << 0 << "\n";
		glerr = true;
	}
	if (!glerr) {
		std::cout << "[RENDER] Compute Visibility Process completed successfully! ----------------------------------\n";
	}

	//__PY_HALT_REQUEST.store(false);
}

SHLM::~SHLM() {
	delete SphericalHarmonics;
}

void SHLM::SetUnifiedHeightmap(Texture* u_heightmap) {
	if (TEXTURE_DIMENSIONALITY != GL_TEXTURE_2D) {
		std::cout << "[RENDER WARNING] Texture dimensions are 3D, to use a heightmap,"
			<<" set SHLM::TEXTURE_DIMENSIONALITY to GL_TEXTURE_2D\n";
		return;
	}
	UnifiedHeightmap = u_heightmap;
}
void SHLM::SetSatelliteTexture(Texture* satelliteTex) {
	if (TEXTURE_DIMENSIONALITY != GL_TEXTURE_2D) {
		std::cout << "[RENDER WARNING] Texture dimensions are 3D, to use a heightmap,"
			<< " set SHLM::TEXTURE_DIMENSIONALITY to GL_TEXTURE_2D\n";
		return;
	}
	SatelliteTex = satelliteTex;
}

void SHLM::SH_renderPass(float FOVdeg, float zNear, float zFar) {

	SH_Program.Activate();
	engine->VAO_1.Bind();

	engine->registerCameraInput(FOVdeg, zNear, zFar);
	if (TEXTURE_DIMENSIONALITY == GL_TEXTURE_2D && UnifiedHeightmap == nullptr) return;
	if (TEXTURE_DIMENSIONALITY != GL_TEXTURE_3D && TEXTURE_DIMENSIONALITY != GL_TEXTURE_2D) {
		std::cout << "[RENDER ERROR] Invalid --> SHLM::TEXTURE_DIMENSIONALITY\n";
		return;
	}

	for (int i = 0; i < 4; i++) {
		// GL_TEXTURE0 expands to 0x84C0, GL_TEXTURE1 to 0x84C1 etc.
		GLenum GL_TEX_UNIT = GL_TEXTURE0 + i;
		glActiveTexture(GL_TEX_UNIT);
		glBindTexture(GL_TEXTURE_2D, vis_tex[i]->GetTexID());
    }

	if (TEXTURE_DIMENSIONALITY == GL_TEXTURE_2D && UnifiedHeightmap != nullptr) {
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, UnifiedHeightmap->GetTexID());
		UnifiedHeightmap->MinMagFilter(GL_LINEAR, GL_LINEAR);
		UnifiedHeightmap->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, SatelliteTex->GetTexID());
	}

	SH_Program.SetUniformVector3("WorldMin", VoxelGrid.GetWorldMin());
	AVector3 WorldScale = VoxelGrid.GetWorldMax() - VoxelGrid.GetWorldMin();
	SH_Program.SetUniformVector3("WorldSInverse", AVector3(1.0f / WorldScale.x, 1.0f / WorldScale.y, 1.0f / WorldScale.z));

	float LightSH[16]; for (int i = 0; i < 16; i++) LightSH[i] = 0.0f;
	AVector3 L = engine->SunCamera.Position.Normalize();

	// Send the Light Normalized Direction for Lambertian shading
	SH_Program.SetUniformVector3("LightDir", L);

	// Send TMY data (DNI, DHI, GHI)
	float dhi = 0.0f; float dni = 0.0f; float ghi = 0.0f;
	if (engine->render_t != -1) {
		dhi = Engine3D::tmy_data.dhi[engine->render_t];
		dni = Engine3D::tmy_data.dni[engine->render_t];
		ghi = Engine3D::tmy_data.ghi[engine->render_t];
	}
	else {
		//std::cout << "t not loaded\n";
	}
	SH_Program.SetFloat("DNI", dni);
	SH_Program.SetFloat("DHI", dhi);
	SH_Program.SetFloat("GHI", ghi);
	SH_Program.SetFloat("MetersPerPixel", 0.3f);

	// Lighting Model Parameters
	SH_Program.SetFloat("DirectShadowStrength", LightParams.DirectShadowStrength);
	SH_Program.SetFloat("AmbientShadowContrast", LightParams.AmbientShadowContrast);
	SH_Program.SetFloat("AmbientShadowIntensity", LightParams.AmbientShadowIntensity);
	SH_Program.SetFloat("DirectLightIntensity", LightParams.DirectLightIntensity);
	SH_Program.SetFloat("AmbientLightIntensity", LightParams.AmbientLightIntensity);

	// Evaluated direction in SH with the ZH convolution normalization constants
	// This function implements DHI scaling on a clear sky model
	SphericalHarmonics->GetLightBasisYUp(L.x, L.y, L.z, LightSH);
	//SphericalHarmonics->ApplyFunkHeckeCosine(LightSH, false);
	SH_Program.SetUniformVec4Array("LightSH", LightSH, 16);

	//float ZHtoSH[4]; SphericalHarmonics->GetCombinedZHtoSH(ZHtoSH);
	//SH_Program.SetUniformVec4Array("ZHtoSH", ZHtoSH, 4);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, engine->window.getWidth(), engine->window.getHeight());

	// 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	engine->DrawAll();

	for (int i = 0; i < 4; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
	
	static bool hadErr = false;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		if (!hadErr) {
			std::cout << "[OPENGL ERROR] Error after Render Shader: 0x" << std::hex << err << " " << std::dec << 0 << "\n";
			hadErr = true;
		}
	}
	
}


void SHLM::ElevateVBO() {

	if (UnifiedHeightmap == nullptr) return;

	Scene* s = engine->getScene();

	uint32_t vertCnt = s->GetVBO_Vector().size();
	if (vertCnt == 0) return;

	ComputeShader DisplaceShader;
	DisplaceShader.Setup("CRenderExtensions/ShadowAnalysis/ComputeShaders/elevate_vbo.comp", "VBO Modify Elevation");
	DisplaceShader.Activate();

	DisplaceShader.SetUniformVector3("WorldMin", worldMin);

	AVector3 worldSize = worldMax - worldMin;
	AVector3 worldSInverse = AVector3(1.0f / worldSize.x, 1.0f / worldSize.y, 1.0f / worldSize.z);
	DisplaceShader.SetUniformVector3("WorldSInverse", worldSInverse);
	DisplaceShader.SetUInt("TotalVertices", vertCnt);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, UnifiedHeightmap->GetTexID());
	//DisplaceShader.SetInt("uHeightmap", 1);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, engine->GetVBO_ID());

	int numGroups = (vertCnt + 255) / 256;
	glDispatchCompute(numGroups, 1, 1);

	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

int SHLM::SkipN_Days = 7;

void SHLM::SumIrradianceOverTime() {

	if (sum_irradiance_in == nullptr || UnifiedHeightmap == nullptr) return;

	float LightSH[16]; for (int i = 0; i < 16; i++) LightSH[i] = 0.0f;
	AVector3 L = engine->SunCamera.Position * 16.0f;
	if (std::abs(L.x) < 0.01f && std::abs(L.y) < 0.01f && std::abs(L.z) < 0.01f) {
		std::cout << "[SOLAR] Degenerated light vector\n";
		return;
	}
	L = L.Normalize();

	//if (engine->render_t < 0) return;
	//if (L.y < 0.0f || Engine3D::tmy_data.ghi[engine->render_t] <= 0.0f) return; // skip night

	SumIrradiance.Activate();

	static int frame_count = 0;

	for (int i = 0; i < 4; i++) {
		// GL_TEXTURE0 expands to 0x84C0, GL_TEXTURE1 to 0x84C1 etc.
		GLenum GL_TEX_UNIT = GL_TEXTURE0 + i;
		glActiveTexture(GL_TEX_UNIT);
		glBindTexture(GL_TEXTURE_2D, vis_tex[i]->GetTexID());
	}

	if (TEXTURE_DIMENSIONALITY == GL_TEXTURE_2D && UnifiedHeightmap != nullptr) {
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, UnifiedHeightmap->GetTexID());
		UnifiedHeightmap->MinMagFilter(GL_LINEAR, GL_LINEAR);
		UnifiedHeightmap->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, SatelliteTex->GetTexID());
	}

	// Send the Light Normalized Direction for Lambertian shading
	SumIrradiance.SetUniformVector3("LightDir", L);

	// Send TMY data (DNI, DHI, GHI)
	float dhi = 0.0f; float dni = 0.0f; float ghi = 0.0f;
	if (engine->render_t != -1) {
		dhi = Engine3D::tmy_data.dhi[engine->render_t];
		dni = Engine3D::tmy_data.dni[engine->render_t];
		ghi = Engine3D::tmy_data.ghi[engine->render_t];
	}
	else {
		//std::cout << "t not loaded\n";
	}
	SumIrradiance.SetFloat("DNI", dni);
	SumIrradiance.SetFloat("DHI", dhi);
	SumIrradiance.SetFloat("GHI", ghi);

	SumIrradiance.SetFloat("SkipN_Days", float(SkipN_Days));

	// Evaluated direction in SH with the ZH convolution normalization constants
	// This function implements DHI scaling on a clear sky model
	SphericalHarmonics->GetLightBasisYUp2(L.x, L.y, L.z, LightSH);
	SumIrradiance.SetUniformVec4Array("LightSH", LightSH, 16);

	GLuint texRead = (frame_count % 2 == 0) ? sum_irradiance_in->GetTexID() : sum_irradiance_out->GetTexID();
	GLuint texWrite = (frame_count % 2 == 0) ? sum_irradiance_out->GetTexID() : sum_irradiance_in->GetTexID();

	frame_count++;

	glBindImageTexture(6, texRead, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(7, texWrite, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

	int w = sum_irradiance_in->GetWidth();
	int h = sum_irradiance_in->GetHeight();

	glDispatchCompute((w + 15) / 16, (h + 15) / 16, 1);
	GLint isBound = 0;
	glGetIntegeri_v(GL_IMAGE_BINDING_LEVEL, 7, &isBound);
	if (isBound < 0) {
		std::cout << "[CRITICAL GPU STATE] Texture write binding 7 is invalid!\n";
	}
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	static bool hadErr = false;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		if (!hadErr) {
			std::cout << "[OPENGL ERROR] Error after Render Shader: 0x" << std::hex << err << " " << std::dec << 0 << "\n";
			hadErr = true;
		}
	}

}


struct TMYHourData {
	float sun_x;
	float sun_y;
	float sun_z;
	float dni;
	float dhi;
	float ghi;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
};
void SHLM::ProcessEntireYear(int total_maxt) {
	std::vector<TMYHourData> gpu_tmy_buffer(total_maxt);
	for (int i = 0; i < total_maxt; i++) {
		gpu_tmy_buffer[i].sun_x = Engine3D::tmy_data.sun_x[i];
		gpu_tmy_buffer[i].sun_y = Engine3D::tmy_data.sun_y[i];
		gpu_tmy_buffer[i].sun_z = Engine3D::tmy_data.sun_z[i];
		gpu_tmy_buffer[i].dni = Engine3D::tmy_data.dni[i];
		gpu_tmy_buffer[i].dhi = Engine3D::tmy_data.dhi[i];
		gpu_tmy_buffer[i].ghi = Engine3D::tmy_data.ghi[i];
	}

	GLuint tmy_ssbo;
	glGenBuffers(1, &tmy_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tmy_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpu_tmy_buffer.size() * sizeof(TMYHourData), gpu_tmy_buffer.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, tmy_ssbo); // Binding = 0 în shader

	SumIrradiance.Activate();
	SumIrradiance.SetInt("TotalHours", total_maxt);
	SumIrradiance.SetFloat("SkipN_Days", float(SkipN_Days));

	for (int i = 0; i < 4; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, vis_tex[i]->GetTexID());
	}
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, UnifiedHeightmap->GetTexID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, BuildingsMaskTexID);

	glBindImageTexture(6, sum_irradiance_out->GetTexID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

	int w = sum_irradiance_out->GetWidth();
	int h = sum_irradiance_out->GetHeight();
	glDispatchCompute((w + 15) / 16, (h + 15) / 16, 1);

	glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
	glFinish();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &tmy_ssbo);
	glBindImageTexture(6, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

	std::cout << "[SOLAR::TMY] Success! Computed all TMY data!\n";
}
