

#include "Engine3D.h"
#include <cstring>

Engine3D* Engine3D::engine = nullptr;

Engine3D* Engine3D::GetEngine3D() {
	if (!engine) engine = new Engine3D();
	return engine;
}

void Engine3D::setCamera(float posX, float posY, float posZ) {
	AVector3 Position = { posX, posY, posZ };
	UserCamera = Camera(Position, -90.0f, 0.0f);
}

void Engine3D::setCamera(float posX, float posY, float posZ, float yaw, float pitch) {
	//EX: Camera({ 0.0f, 80.0f, 120.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });;
	AVector3 Position = { posX, posY, posZ };
	UserCamera = Camera(Position, yaw, pitch);
}

void Engine3D::setSunCamera(float posX, float posY, float posZ) {
	//EX: Camera({ 0.0f, 80.0f, 120.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });;
	AVector3 Position = { posX, posY, posZ };
	SunCamera = Camera(Position, 0.0f, 0.0f);
}


void Engine3D::DEBUG_showCameraVectors() {
	std::cout << UserCamera.Position.x << " " << UserCamera.Position.y << " " << UserCamera.Position.z<<"\n";
	std::cout << UserCamera.Yaw <<" "<< UserCamera.Pitch << "\n";
}

int Engine3D::setupGLFW(const int WINDOW_WIDTH, const int WINDOW_HEIGHT, const char * WINDOW_TITLE) {
	// INITIALIZE GLFW
	glfwInit();
	// SOME SPECS FOR OUR OPENGL VERSION THAT WE SEND TO GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//CORE profile from OPENGL; only modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//Create the WINDOW OBJECT with our defined width and height and title
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	//Error checking if anything went wrong in the window creating process
	if (window == NULL) {
		std::cout << "Error when creating window \n";
		glfwTerminate();
		return 0; //
	}
	//Tell GLFW we are using our created window as it's context
	glfwMakeContextCurrent(window);

	//Load GLAD (needed to configure OpenGL)
	gladLoadGL();
	//Specify Viewport to OpenGL ( from (0,0) to (W_WIDTH,W_HEIGHT) )
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	windowWidth = WINDOW_WIDTH;
	windowHeight = WINDOW_HEIGHT;
	windowAspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
	return 1;
}

const int Engine3D::getDrawStyle(const char* style) {
	if (strcmp(style, "static") == 0) return GL_STATIC_DRAW;
	if (strcmp(style, "stream") == 0) return GL_STREAM_DRAW;
	if (strcmp(style, "dynamic") == 0) return GL_DYNAMIC_DRAW;
	return GL_STATIC_DRAW;
}

void Engine3D::setupShaders() {

	//if (UserCamera == nullptr) { std::cerr << "Unable to setup Shader before Camera Object, please call setCamera() first \n"; return; }

	shaderProgram.Setup("Shaders/default.vert", "Shaders/default.frag");

	instanceProgram.Setup("Shaders/instance.vert", "Shaders/instance.frag");

	//std::cout << "Shader setup complete! \n";

	// DEFAULT SHADER PROGRAM

	//lightDirUnifLoc = glGetUniformLocation(shaderProgram.ID, "lightDirection");

	// INSTANCE SHADER PROGRAM

	//lightDirUnifLoc = glGetUniformLocation(instanceProgram.ID, "lightDirection");

	glEnable(GL_DEPTH_TEST);

}

void Engine3D::setupGeometryArrayObjects(const int drawStyle = GL_STATIC_DRAW) {
	//std::cout << "VAO Setup \n";
	VAO_1.Setup();

	VAO_1.Bind();
	//std::cout << "VAO Setup and Binding complete \n";
	/*
	std::vector<AVertex>& worldVertices = MainScene.getVertStoreLocation().getWorldVertices();

	std::vector<GLuint>& VertIndicies = MainScene.getVertStoreLocation().getVertIndicies();
	*/
	std::cout << "Got vertex and indicies buffers \n";

	std::vector<AVertex>& vert = MainScene.GetVBO_Organizer().GetMultiArray();
	std::vector<GLuint>& indicies = MainScene.GetEBO_Organizer().GetMultiArray();
	VBO_1.Setup(vert.data(), vert.size() * sizeof(AVertex), drawStyle);
	EBO_1.Setup(indicies.data(), indicies.size() * sizeof(GLuint), drawStyle);

	std::cout << "Total VBO elements: " << vert.size() << "\n";

	std::cout << "VBO & EBO setup complete \n";

	GLsizei stride = sizeof(AVertex); //32 bytes
	
	// APosition ( 3 floats)
	VAO_1.LinkVBO(VBO_1, 0, 3, GL_FLOAT, stride, GL_FALSE, (void*)0);
	// RGBA	( uint32 = 4 * byte )
	VAO_1.LinkVBO(VBO_1, 1, 4, GL_UNSIGNED_BYTE, stride, GL_TRUE, (void*)12);
	// ANormal ( 3 floats )
	VAO_1.LinkVBO(VBO_1, 2, 3, GL_FLOAT, stride, GL_FALSE, (void*)16);
	// UV ( uint32 = short + short )
	VAO_1.LinkVBO(VBO_1, 3, 2, GL_UNSIGNED_SHORT, stride, GL_TRUE, (void*)28);

	std::cout << "VBO linking complete \n";

	depthTextureObject.setupFBO();
	depthTextureObject.setupDepthTexture(2048);

	//std::cout << "Setup FBO complete \n";

	VAO_1.Unbind();
	VBO_1.Unbind();
	EBO_1.Unbind();

	//std::cout << "Unbinding..\n";
}

void Engine3D::setupInstanceVBO() {

	// We create a VBO for matrices of instances
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// We specify GL_DYNAMIC_DRAW beacause our values will change frequently as the camera moves
	// We need a new relative translation matrix every frame for every instance,
	// Even though our camera view and proj matrix stay the same for every instance

	int matarraysize = MainScene.GetInstanceOrganizer().GetMultiArray().size();

	glBufferData(GL_ARRAY_BUFFER, matarraysize * sizeof(InstanceData), NULL, GL_DYNAMIC_DRAW);

	// Configure VAO for mat4 matricies (4*vec4 size)
	VAO_1.Bind();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	for (int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(4 + i);
		glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(sizeof(glm::vec4) * i));
		glVertexAttribDivisor(4 + i, 1);
	}
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(InstanceData), (void*)64);
	glVertexAttribDivisor(8, 1);
	glEnableVertexAttribArray(9);
	glVertexAttribPointer(9, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(InstanceData), (void*)68);
	glVertexAttribDivisor(9, 1);
	VAO_1.Unbind();

}

void Engine3D::DrawInstances(Blueprint* BLUEPRINT, Tile* TILE) {

	if (TILE == nullptr) return;

	try {

		int HandleID = BLUEPRINT->GetID() | (TILE->GetTileID() << Tile::shiftComponent);
		Handle InstancesHandle = MainScene.GetInstanceOrganizer().GetHandleData(HandleID);

		Handle BlueprintHandle = MainScene.GetBlueprintHandle(BLUEPRINT, EBO_ORGANIZER_TARGET);

		void* offsetPtr = (void*)(uintptr_t)(BlueprintHandle.offset * sizeof(GLuint));

		glDrawElementsInstanced(GL_TRIANGLES, BlueprintHandle.size, GL_UNSIGNED_INT, offsetPtr, InstancesHandle.size);
	}
	catch (ArrayOrganizerException& e) {
		std::cout << e.what() << "\n";
	}
}

Camera& Engine3D::getCamera(bool Sun) { if (Sun) { return SunCamera; } return UserCamera; }
Scene* Engine3D::getScene() { return &MainScene; }

void Engine3D::initGameFrame() {
	//Set background color to be drawn
	glClearColor(backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);
}


void Engine3D::registerCameraInput(float FOVdeg, float zNear, float zFar) {
	UserCamera.Inputs(window);
	//UserCamera.Matrix(FOVdeg, zNear, zFar, windowAspectRatio, shaderProgram);
	UserCamera.Matrix(FOVdeg, zNear, zFar, windowAspectRatio, instanceProgram);
}

void Engine3D::DrawAllInstances() {
	std::vector<int> handleIDsFromRoot;
	MainScene.WorldRoot->RecurseInTilesOutputHandleIDs(handleIDsFromRoot);
	for (int i = 0; i < handleIDsFromRoot.size(); i++) {
		int HandleID = handleIDsFromRoot[i];

		Handle InstH = MainScene.GetInstanceOrganizer().GetHandleData(HandleID);

		Handle BlueprintHandle = MainScene.GetEBO_Organizer().GetHandleData(HandleID & 4095);

		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		uint32_t stride = sizeof(InstanceData);
		if (stride != 72) std::cout << "ALERT!";
		uintptr_t base = (uintptr_t)(InstH.offset * stride);
		for (int j = 0; j < 4; j++) {
			glEnableVertexAttribArray(4 + j);
			glVertexAttribPointer(4 + j, 4, GL_FLOAT, GL_FALSE, stride, (void*)(base + j * 16));
			glVertexAttribDivisor(4 + j, 1);
		}

		uint32_t offRGBA = 64;
		uint32_t offUV = 68;

		glEnableVertexAttribArray(8);
		void* offset = (void*)(uintptr_t)(InstH.offset * stride + offRGBA);
		glVertexAttribPointer(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offset);
		glVertexAttribDivisor(8, 1);
		glEnableVertexAttribArray(9);
		offset = (void*)(uintptr_t)(InstH.offset * stride + offUV);
		glVertexAttribPointer(9, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, offset);
		glVertexAttribDivisor(9, 1);

		void* offsetPtr = (void*)(uintptr_t)(BlueprintHandle.offset * sizeof(GLuint));

		Handle VBO_Handle = MainScene.GetVBO_Organizer().GetHandleData(HandleID & 4095);

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, BlueprintHandle.size, GL_UNSIGNED_INT, 
			offsetPtr, InstH.size, VBO_Handle.offset);

		//std::cout << "\n //////Rendering HandleID: " << HandleID << " with " << InstH.size << " instances. \n\n";

	}
}

void Engine3D::shadowPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, depthTextureObject.FBO_ID);
	glViewport(0, 0, 2048, 2048);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	
	shaderProgram.Activate();
	VAO_1.Bind();

	glUniform3f(shaderProgram.GetUniformLocation("CamPosition"), 0, 0, 0);

	SunCamera.LightMatrix(500.0f, shaderProgram, false);

	// The Sun is looking from it's Position to the center (0,0,0);
	glUniform3f(shaderProgram.GetUniformLocation("lightDirection"), SunCamera.Position.x, SunCamera.Position.y, SunCamera.Position.z);

	//glDrawElements(GL_TRIANGLES, indiciesSize, GL_UNSIGNED_INT, 0);


	instanceProgram.Activate();
	VAO_1.Bind();

	int matarraysize = MainScene.GetInstanceOrganizer().GetMultiArray().size();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, matarraysize * sizeof(InstanceData), &MainScene.GetInstanceOrganizer().GetMultiArray()[0]);

	glUniform3f(instanceProgram.GetUniformLocation("CamPosition"), 0, 0, 0);

	SunCamera.LightMatrix(500.0f, instanceProgram, true);

	glUniform3f(instanceProgram.GetUniformLocation("lightDirection"), SunCamera.Position.x, SunCamera.Position.y, SunCamera.Position.z);

	DrawAllInstances();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
}

void Engine3D::renderPass(float FOVdeg, float zNear, float zFar) {

	this->registerCameraInput(FOVdeg, zNear, zFar);

	glViewport(0, 0, windowWidth, windowHeight);
	//Clear the BACK BUFFER and assign our color to it
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);

	

	shaderProgram.Activate();
	VAO_1.Bind();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	SunCamera.LightMatrix(500.0f, shaderProgram, true);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTextureObject.depthTexture);

	glUniform1i(shadowMapLocation, 0);

	//glDrawElements(GL_TRIANGLES, indiciesSize, GL_UNSIGNED_INT, 0);

	

	// START TO DRAW INSTANCES

	instanceProgram.Activate();
	VAO_1.Bind();

	int matarraysize = MainScene.GetInstanceOrganizer().GetMultiArray().size();
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, matarraysize * sizeof(InstanceData), &MainScene.GetInstanceOrganizer().GetMultiArray()[0]);

	SunCamera.LightMatrix(500.0f, instanceProgram, true);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTextureObject.depthTexture);

	glUniform1i(glGetUniformLocation(instanceProgram.ID, "shadowMap"), 0);

	// TODO -- INSTANCE SHADER CONFIGURE, UNIFORMS IN OUR SAHDER
	//		-- INSTANCE DATA FORWARDING & FORMATING
	//		-- INSTANCE RENDERING
	DrawAllInstances();

	//Swap BACK BUFFER with FRONT BUFFER
	glfwSwapBuffers(window);
	// Get events (for controls, event handling, closing, etc.)
	glfwPollEvents();
}

Tile* Engine3D::getVisibleCameraFrustum() {
	return MainScene.FindTileForPosition(
		AVertex(), UserCamera.Position
	);
}

void Engine3D::EngineTerminate() {

	/*
	//Delete meshes
	for (auto& MeshObj : MainScene.getMeshes()) {
		delete MeshObj;
	}
	*/

	//Delete our VAOs, VBOs, EBOs
	VAO_1.Delete();
	VBO_1.Delete();
	EBO_1.Delete();

	//Delete instanceVBO
	glDeleteBuffers(1, &instanceVBO);

	//Delete shader
	shaderProgram.Delete();

	//Destroy WINDOW OBJECT
	glfwDestroyWindow(window);
	//Terminate GLFW
	glfwTerminate();
}

Blueprint* Engine3D::LoadSTLGeomFile(const char* fileName, float scale) {
	std::vector<float> coords, normals;
	std::vector<unsigned int> tris, solids;

	try {
		stl_reader::ReadStlFile(fileName, coords, normals, tris, solids);

		std::vector<AVertex> vert;
		std::vector<GLuint> indicies;

		// Avoid duplicate verticies by using a map from the
		// Original vertex index in the STL file to 
		// A new index to be put in indicies 
		// (inserted even if vertex index is already in map)
		std::unordered_map<int, GLuint> uniqueVert;

		const size_t totalIndices = tris.size();

		std::cout <<"Mesh coord count: " << coords.size() << " trig count: " << tris.size()<<"\n";

		for (int i = 0; i < totalIndices; i++) {
			int STLfileIndex = tris[i];

			if (uniqueVert.find(STLfileIndex) == uniqueVert.end()) {
				// Found a unique vertex that is not a duplicate
				// Add to our map
				uniqueVert[STLfileIndex] = vert.size();

				int coordINDEX = 3 * STLfileIndex;
				float* c = &coords[coordINDEX];

				vert.push_back(AVertex(c[0] * scale, c[1] * scale, c[2] * scale, 200, 200, 200, 255));
			}

			indicies.push_back(uniqueVert[STLfileIndex]);
		}

		std::cout << "Mesh created \n";

		Blueprint::CalculateSurfaceNormals(vert, indicies);

		return MainScene.CreateBlueprint(vert, indicies);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return nullptr;
}

Blueprint* Engine3D::CreatePrism(const std::vector<AVertex>& vertices, int VertexNumber, float height) {

	std::vector<GLuint> indicies;

	std::vector<AVertex> V = vertices;
	V.reserve(VertexNumber * 2);

	if (vertices.size() < 3) { std::cout << "Mesh does not contain any triangles \n"; return nullptr; };

	// 0 -> 1 -> 2
	// |  / |  / |
	// | /  | /  |
	// 3 -> 4 -> 5

	for (int i = 0; i < VertexNumber; i++) {
		AVertex vclone = vertices[i];
		vclone.POS.y += height;
		V.push_back(vclone); // create bottom vertex
	}
	for (int i = 0; i < VertexNumber-1; i++) {
		//LATERAL FACE 1
		indicies.push_back(i);
		indicies.push_back(VertexNumber + i);
		indicies.push_back(i + 1);
		//LATERAL FACE 2
		indicies.push_back(VertexNumber + i);
		indicies.push_back(VertexNumber + i + 1);
		indicies.push_back(i + 1);
	}

	//LATERAL FACE 1
	indicies.push_back(VertexNumber - 1);
	indicies.push_back(2 * VertexNumber - 1);
	indicies.push_back(0);
	//LATERAL FACE 2
	indicies.push_back(VertexNumber);
	indicies.push_back(0);
	indicies.push_back(2 * VertexNumber - 1);

	for (int i = 1; i < VertexNumber-1; i++) {
		//BOTTOM FACE
		indicies.push_back(0);
		indicies.push_back(i + 1);
		indicies.push_back(i);
		
		//TOP FACE
		indicies.push_back(VertexNumber);
		indicies.push_back(VertexNumber + i + 1);
		indicies.push_back(VertexNumber + i);
	}

	return MainScene.CreateBlueprint(V, indicies);
}

Blueprint* Engine3D::CreateRectPrism(float length, float width, float height) {
	std::vector<AVertex> v;
	v.resize(4);
	v[0] = AVertex( -length / 2.0f, -height / 2.0f, -width / 2.0f, 200, 200, 200, 255);
	v[1] = AVertex( length / 2.0f, -height / 2.0f, -width / 2.0f, 200, 200, 200, 255);
	v[2] = AVertex( length / 2.0f, -height / 2.0f, width / 2.0f, 200, 200, 200, 255);
	v[3] = AVertex( -length / 2.0f, -height / 2.0f, width / 2.0f, 200, 200, 200, 255);
	return CreatePrism(v, 4, height);
}

Blueprint* Engine3D::CreateCube(float length) {
	return CreateRectPrism(length, length, length);
}
