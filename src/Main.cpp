#include <iostream>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "window";
const float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

#include "Engine3D.h"

int main() {

	Engine3D& engine = *Engine3D::GetEngine3D();
	std::cout << "Engine initialized \n";

	int success = engine.setupGLFW(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
	if (!success) { std::cerr << "Error at setup \n"; return -1; }

	engine.setBackground(0.2f, 0.3f, 0.8f, 1.0f);

	// HERE WE CREATE OUR OBJECTS /////////////////////////////////////////////////
	
	Blueprint* humanMesh = engine.LoadSTLGeomFile("resources/BASEmodel.stl", 20.0f);
	if (humanMesh) {
		std::cout << "Created: Human \n";
		for (int i = 0; i < 10; i++) {
			Instance* human = engine.getScene()->CreateInstance(humanMesh, AVector3());
			human->SetPosition(AVector3(-160.0f + i * 40.0f, 50.0f, 5.0f));
			human->SetColor(i * 20, 255 - i * 20, 0, 255);
			human->SetRotation(AVector3(-90.0f, 0.0f, 0.0f));
		}
	}
	
	Blueprint* cubeB = engine.CreateCube(10.0f);

	for (int i = 0; i < 10; i++) {
		Instance* I = engine.getScene()->CreateInstance(cubeB, AVector3());
		I->SetColor(0, i*15, 255, 255);
		I->SetPosition(AVector3(10.0f * i, 1.0f * i, 10.0f * i));
	}

	Instance* plane = engine.getScene()->CreateInstance(cubeB, AVector3());
	plane->SetPosition(AVector3(0.0f, -5.0f, 0.0f));
	plane->SetSize(AVector3(20.0f, 1.0f, 20.0f));
	plane->SetColor(AColor3(50, 255, 25, 255));

	std::vector<AVertex> Verticies = { AVertex(-4.8f, 0.0f, -4.0f), AVertex(3.5f, 0.0f, -3.75f), AVertex(1.6f, 0.0f, 4.15f), AVertex(-2.65f, 0.0f, 2.75f) };
	Blueprint* prismB = engine.CreatePrism(Verticies, 4, 10.0f);

	Instance* triPrism = engine.getScene()->CreateInstance(prismB, AVector3());
	triPrism->SetColor(255, 255, 0, 255);
	triPrism->SetPosition(AVector3(-10.0f, -5.0f, 5.0f));
	triPrism->SetSize(AVector3(5.0f, 5.0f, 5.0f));
	
	////////////////////////////////////////////////////////////////////////////////

	std::cout << "Meshes constructed \n";

	engine.setCamera(0.0f, 80.0f, 120.0f); //Set camera to this position
	engine.setSunCamera(0.0f, 100.0f, 1.0f);

	engine.setupShaders(); //Uses Camera Class and Mesh Instances

	std::cout << "VBO Total Bytes: " << engine.getScene()->GetVBO_Organizer().GetMultiArray().size() * sizeof(AVertex) << "\n";
	std::cout << "EBO Total Bytes: " << engine.getScene()->GetEBO_Organizer().GetMultiArray().size() * sizeof(GLuint) << "\n";


	engine.setupGeometryArrayObjects(engine.getDrawStyle("dynamic"));
	engine.setupInstanceVBO();

	std::cout << "Shaders created\n";

	// TEST: engine.getScene()->GetTileMeshes(engine.getScene()->WorldRoot->Divisions[1][1]);
	
	//engine.setupInstanceVBO(cntOfObj);

	
	//glDisable(GL_CULL_FACE);

	float _deltaTimeForTIMER = 1.0f / 10.0f;
	double prevTime = glfwGetTime();

	int ROT = 0;

	//FOR FPS COUNTER
	double PREV_TIME = 0.0f;
	double CURRENT_TIME = 0.0f;
	double timeDifference;
	int frameCounter = 0;
	const double FPSsampleTime = 1.0f / 20.0f;

	std::cout << "Instances Size: " << engine.getScene()->GetInstanceOrganizer().GetMultiArray().size() << std::endl;
	std::cout << "VBO Size: " << engine.getScene()->GetVBO_Organizer().GetMultiArray().size() << std::endl;
	std::cout << "EBO Size: " << engine.getScene()->GetEBO_Organizer().GetMultiArray().size() << std::endl;


	// MAIN GAME LOOP
	while (!engine.windowShouldClose()) {

		CURRENT_TIME = glfwGetTime();
		timeDifference = CURRENT_TIME - PREV_TIME;
		frameCounter++;
		if (timeDifference >= FPSsampleTime) {
			std::string FPS = std::to_string((1.0f / timeDifference) * frameCounter);
			std::string msPerFrame = std::to_string((timeDifference / frameCounter) * 1000);
			std::string winTitle = "WINDOW | " + FPS + " FPS | " + msPerFrame + " ms/frame";
			glfwSetWindowTitle(engine.getWindow(), winTitle.c_str());
			PREV_TIME = CURRENT_TIME;
			frameCounter = 0;
		}


		engine.initGameFrame(); // setting up the background color

		// TIMER (global) ////////////
		double currentTime = glfwGetTime();
		if (currentTime - prevTime >= _deltaTimeForTIMER) {
			//engine.DEBUG_showCameraVectors();
			prevTime = currentTime;
			/*
			if (humanMesh) {
				humanMesh->Rotation = AVector3((-(int)sqrt(ROT*20) % 360 - 180) * 1.0f, 
					(ROT % 360 - 180) * 1.0f, (-ROT % 360 - 180) *1.0f);
				humanMesh->UpdVectors();
			}
			*/
			float sinROT = sin((double)ROT / 1024.0f);
			float cosROT = cos((double)ROT / 1024.0f);
			engine.getCamera(true).Position = AVector3( 100.0f * cosROT, 100.0f * sinROT, 50.0f );
			float lightReflactance = std::max(0.0f, sinf((float)ROT / 512.0f));
			float sunsetCoef = abs(cos((double)ROT / 512.0f)*1.5f);
			engine.setBackground((sunsetCoef) * 0.25f * (lightReflactance + 0.25f), 
				(lightReflactance + sunsetCoef / 2.0f) * 0.5f * (lightReflactance), 
				lightReflactance, 1.0f);
			ROT+=4;
		}

		engine.shadowPass();
		engine.renderPass(45.0f, 0.1f, 1000.0f);
	}

	return 0;
}