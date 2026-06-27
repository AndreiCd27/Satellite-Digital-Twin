

#include "Engine3D.h"
#include <cstring>

Engine3D* Engine3D::engine = nullptr;

Engine3D* Engine3D::GetEngine3D() {
	if (!engine) engine = new Engine3D();
	return engine;
}

void Engine3D::setWindowTitle(const std::string& winTitle) {
	glfwSetWindowTitle(window.getWindow(), winTitle.c_str());
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
void Engine3D::DEBUG_ArrayOrganizers() {

	std::cout << "VBO Total Bytes: " << getScene()->GetVBOsize() * sizeof(AVertex) << "\n";
	std::cout << "EBO Total Bytes: " << getScene()->GetEBOsize() * sizeof(GLuint) << "\n";

	std::cout << "\nPrinting VBO_Organizer... \n";
	getScene()->PrintVBO();
	std::cout << "Printing EBO_Organizer... \n";
	getScene()->PrintEBO();

}

int Engine3D::setupWindow(const int WINDOW_WIDTH, const int WINDOW_HEIGHT, const char * WINDOW_TITLE) {
	
	bool success = window.CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	if (!success) {
		std::cout << "Error when creating window \n";
		EngineTerminate();
		return -1;
	}
	//Load GLAD (needed to configure OpenGL)
	gladLoadGL();
	//Specify Viewport to OpenGL ( from (0,0) to (W_WIDTH,W_HEIGHT) )
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	cfg.Exec(WINDOW_SETUP_STAGE);

	return 1;
}

int Engine3D::getDrawStyle(const char* style) {
	if (strcmp(style, "static") == 0) return GL_STATIC_DRAW;
	if (strcmp(style, "stream") == 0) return GL_STREAM_DRAW;
	if (strcmp(style, "dynamic") == 0) return GL_DYNAMIC_DRAW;
	return GL_STATIC_DRAW;
}

void Engine3D::setupShaders() {

	glGenVertexArrays(1, &emptyVAO);
	DebugTexShader.Setup("Shaders/debug_tex.vert", "Shaders/debug_tex.frag");

	instanceProgram.Setup("Shaders/instance.vert", "Shaders/instance.frag");

	shadowProgram.Setup("Shaders/shadow.vert", "Shaders/shadow.frag");

	glEnable(GL_DEPTH_TEST);

	cfg.Exec(SHADER_SETUP_STAGE);

}

void Engine3D::setupGeometryArrayObjects(const char* style) {
	
	const int drawStyle = getDrawStyle(style);

	VAO_1.Setup();
	VAO_1.Bind();

	if (DEBUG)std::cout << "Got vertex and indicies buffers \n";

	auto vert = MainScene.GetVBO_Vector();
	auto indicies = MainScene.GetEBO_Vector();

	VBO_1.Setup(vert.data(), vert.size() * sizeof(AVertex), drawStyle);
	EBO_1.Setup(indicies.data(), indicies.size() * sizeof(GLuint), drawStyle);

	if (DEBUG) {
		std::cout << "Total VBO elements: " << vert.size() << "\n";
		std::cout << "VBO & EBO setup complete \n";
	}

	GLsizei stride = sizeof(AVertex); //32 bytes
	
	// APosition ( 3 floats)
	VAO_1.LinkVBO(VBO_1, 0, 3, GL_FLOAT, stride, GL_FALSE, voidcast(0));
	// RGBA	( uint32 = 4 * byte )
	VAO_1.LinkVBO(VBO_1, 1, 4, GL_UNSIGNED_BYTE, stride, GL_TRUE, voidcast(12));
	// ANormal ( 3 floats )
	VAO_1.LinkVBO(VBO_1, 2, 3, GL_FLOAT, stride, GL_FALSE, voidcast(16));
	// UV ( uint32 = short + short )
	VAO_1.LinkVBO_int(VBO_1, 3, 1, GL_UNSIGNED_INT, stride, voidcast(28));

	if (DEBUG)std::cout << "VBO linking complete \n";

	depthTextureObject.setupFBO();
	depthTextureObject.setupDepthTexture(4096*2);

	if (DEBUG)std::cout << "Setup FBO complete \n";

	VAO_1.Unbind();
	VBO_1.Unbind();
	EBO_1.Unbind();

	if (DEBUG)std::cout << "Unbinding..\n";

	cfg.Exec(GEOMETRY_ARRAY_OBJECTS_SETUP_STAGE);
}

void Engine3D::SetupFull(const char* drawStyle) {
	engine->setCamera(0.0f, 40.0f, 120.0f); // Default Player Camera Position
	engine->setSunCamera(200.0f, 200.0f, 200.0f); // Default Sun Camera Position

	// Configure shaders
	engine->setupShaders();

	// Configure OpenGL essentials:
	// 1) VAO (Vertex Array Object) - here we store "layouts"; we bind with VBO, EBO, instanceVBO
	// 2) VBO (Vertex Buffer Object) - here we store Blueprint vertices and send them to OpenGL
	// 3) EBO (Entity Buffer Object) - here we store vertex indicies, such that OpenGL knows
	engine->setupGeometryArrayObjects(drawStyle);
}

Camera& Engine3D::getCamera(bool Sun) { if (Sun) { return SunCamera; } return UserCamera; }
Scene* Engine3D::getScene() { return &MainScene; }

void Engine3D::setSunAzimuthAndElevation(float azimuth, float elevation, float R) {
	//std::cout << "ELEVATION: " << elevation << " AZIMUTH: " << azimuth<<"\n";
	float x = R * cosf(elevation) * sinf(azimuth);
	float z = R * cosf(elevation) * cosf(azimuth);
	float y = R * sinf(elevation);
	SunCamera.Position = AVector3(x, y, z);
}

bool Engine3D::SunFromTMY(int t) {
	if (tmy_data.sun_x.size() == 0) {
		SunCamera.Position = AVector3(0.0f, 1.0f, 0.0f);
	}
	else {
		if (t < tmy_data.sun_x.size() && t >= 0) {
			render_t = t;
			SunCamera.Position = AVector3(tmy_data.sun_x[t], tmy_data.sun_y[t], tmy_data.sun_z[t]);
			/*
			std::cout << "[SUN POSITION]" << SunCamera.Position.x
				<< " " << SunCamera.Position.y << " " << SunCamera.Position.z << "\n";
			*/
		}
		else {
			return false;
		}
	}
	return true;
}

void Engine3D::initGameFrame() {

	// Framecounter
	double CURRENT_TIME = glfwGetTime();
	double timeDifference = CURRENT_TIME - FPS.PREV_TIME;
	FPS.frameCounter++;
	if (timeDifference >= FPS.FPSsampleTime) {
		std::string strFPS = std::to_string((1.0f / timeDifference) * FPS.frameCounter);
		FPS.msPerFrame = (timeDifference / FPS.frameCounter) * 1000.0f;
		std::string msPerFrame = std::to_string(FPS.msPerFrame);
		std::string winTitle = "WINDOW | " + strFPS + " FPS | " + msPerFrame + " ms/frame";
		setWindowTitle(winTitle);
		FPS.PREV_TIME = CURRENT_TIME;
		FPS.frameCounter = 0;
	}
	// Set background color to be drawn
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);

	cfg.Exec(INIT_GAME_FRAME_STAGE);

}


void Engine3D::registerCameraInput(float FOVdeg, float zNear, float zFar) {

	UserCamera.Matrix(FOVdeg, zNear, zFar, window.getAspectRatio(), instanceProgram);

	if (cfg.CameraOverride == true) {
		cfg.Exec(CAMERA_INPUT_STAGE);

		return;
	}

	UserCamera.Inputs(window.getWindow(), FPS.msPerFrame);

	cfg.Exec(CAMERA_INPUT_STAGE);
}

void Engine3D::DrawAll() {

	VAO_1.Bind();

	glDrawElements(
		GL_TRIANGLES,
		totalIndices,
		GL_UNSIGNED_INT,
		nullptr
	);

	VAO_1.Unbind();
}

void Engine3D::shadowPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, depthTextureObject.GetFBO_ID());
	glViewport(0, 0, 4096, 4096);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	shadowProgram.Activate();
	VAO_1.Bind();

	// The sun looks from the UserCamera's position, so the shadowsMap doesn't stay forever at (0,0,0)
	SunCamera.LightMatrix(500.0f, shadowProgram, false, UserCamera.Position);

	DrawAll();
	
}

void Engine3D::renderPass(float FOVdeg, float zNear, float zFar) {
	
	instanceProgram.Activate();
	VAO_1.Bind();

	this->registerCameraInput(FOVdeg, zNear, zFar);
	instanceProgram.SetUniformVector3("lightDirection", SunCamera.Position);
	// The sun looks from the UserCamera's position, so the shadowsMap doesn't stay forever at (0,0,0)
	SunCamera.LightMatrix(500.0f, instanceProgram, true, UserCamera.Position);

	glUniform1i(instanceProgram.GetUniformLocation("shadowMap"), 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glViewport(0, 0, window.getWidth(), window.getHeight());

	glCullFace(GL_BACK);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate Depth Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthTextureObject.GetTexID());
	
	DrawAll();
	
	glDisable(GL_BLEND);
}

void Engine3D::UpdateBuffers() {
	if (MainScene.GetUpdateStatus()) {
		std::cout << "+++++++++ UPDATE BUFFERS ++++++++++++++++++++\n";

		std::lock_guard<std::mutex> lock(MainScene.GetMutex());

		auto& vert = MainScene.GetVBO_Vector();
		auto& indicies = MainScene.GetEBO_Vector();

		if (vert.empty() || indicies.empty()) return;

		VBO_1.Bind();
		glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(AVertex), vert.data(), GL_STATIC_DRAW);

		EBO_1.Bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(GLuint), indicies.data(), GL_STATIC_DRAW);

		totalIndices = (int)indicies.size();

		MainScene.ResetUpdateStatus();
	}
}

void Engine3D::Render() {

	if (DEBUG_TEXTURE && debug_texture_target != nullptr) {
		RenderDebugTexture(*debug_texture_target);
		glfwSwapBuffers(window.getWindow());
		glfwPollEvents();
		return;
	}

	if (cfg.PreRenderRequest != nullptr) {
		//std::cout << "Executing Pre Render Request!\n";
		cfg.PreRenderRequest->Exec(); 
	}

	if (cfg.RenderOverride) {

		cfg.Exec(RENDER_INSTANCES_STAGE);

		//std::cout << "Executed Render Override \n";

		if (cfg.PostRenderRequest != nullptr) {
			//std::cout << "Executing Post Render Request!\n";
			cfg.PostRenderRequest->Exec();
		}

		// IMGUI Connection
		cfg.Exec(GUI_INSTANCES_STAGE);
		
		//Swap BACK BUFFER with FRONT BUFFER
		glfwSwapBuffers(window.getWindow());
		// Get events (for controls, event handling, closing, etc.)
		glfwPollEvents();

		return;
	}

	shadowPass();
	renderPass(45.0f, 0.1f, 5000.0f);

	if (cfg.PostRenderRequest != nullptr) {
		//std::cout << "Executing Post Render Request!\n";
		cfg.PostRenderRequest->Exec();
	}

	// IMGUI Connection
	cfg.Exec(GUI_INSTANCES_STAGE);

	//Swap BACK BUFFER with FRONT BUFFER
	glfwSwapBuffers(window.getWindow());
	// Get events (for controls, event handling, closing, etc.)
	glfwPollEvents();

}

/* FUNCTIONS MAY BE USED AT A LATER TIME WHEN RENDERING ACCOUNTS FOR VISIBLE TILES
Tile* Engine3D::getVisibleCameraFrustum() {
	return MainScene.FindTileForPosition(
		AVertex(), UserCamera.Position
	);
}
*/

void Engine3D::EngineTerminate() {

	//Delete our VAOs, VBOs, EBOs
	if (engine->VAO_1.GetCompleteStatus()) engine->VAO_1.Delete();
	if (engine->VBO_1.GetCompleteStatus()) engine->VBO_1.Delete();
	if (engine->EBO_1.GetCompleteStatus()) engine->EBO_1.Delete();

	//Delete shader
	if (engine->instanceProgram.GetCompleteStatus()) engine->instanceProgram.Delete();

	//Destroy WINDOW OBJECT
	engine->window.Terminate();

	delete engine;

}

bool Engine3D::DEBUG_TEXTURE = false;
int Engine3D::DEBUG_TEXTURE_CHANNELS = 3;
Texture* Engine3D::debug_texture_target = nullptr;
float Engine3D::DEBUG_TEXTURE_SCALAR = 1.0f;

void Engine3D::RenderDebugTexture(const Texture& texToDebug) {
	
	glDisable(GL_DEPTH_TEST);

	DebugTexShader.Activate();

	DebugTexShader.SetFloat("scale", DEBUG_TEXTURE_SCALAR);

	DebugTexShader.SetInt("debug_tex", 0);
	DebugTexShader.SetInt("channels", DEBUG_TEXTURE_CHANNELS);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texToDebug.GetTexID());

	glBindVertexArray(emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}

TMY_DATA Engine3D::tmy_data;
