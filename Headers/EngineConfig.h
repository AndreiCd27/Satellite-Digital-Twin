#pragma once

#include "Action.h"
#include "Request.h"

#include <stdexcept>
#include <iostream>

constexpr int DEF_STAGES = 7;

// Stage 0 ---> Engine3D::SetupWindow()
constexpr int WINDOW_SETUP_STAGE = 0;

// Stage 1 ---> Engine3D::SetupShaders()
constexpr int SHADER_SETUP_STAGE = 1;

// Stage 2 ---> Engine3D::setupGeometryArrayObjects()
constexpr int GEOMETRY_ARRAY_OBJECTS_SETUP_STAGE = 2;

// Stage 3 ---> Engine3D::setupInstanceVBO()
constexpr int INSTANCE_ARRAY_OBJECTS_SETUP_STAGE = 3;

// Stage 4 ---> Engine3D::initGameFrame()
constexpr int INIT_GAME_FRAME_STAGE = 4;

// Stage 5 ---> Engine3D::registerCameraInput()
constexpr int CAMERA_INPUT_STAGE = 5;

// Stage 6 ---> Engine3D::RenderInstances()
constexpr int RENDER_INSTANCES_STAGE = 6;

class ConfigException : public std::runtime_error {
public:
	ConfigException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ ARRAY_ORGANIZER_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

class EngineConfig {

	ActionContainer ActionStages[DEF_STAGES];

	std::vector<int> ExecOrder[DEF_STAGES];
	bool ExecOrderSpecified = false;

public:

	AbstractFunc* PreRenderRequest = nullptr;
	AbstractFunc* PostRenderRequest = nullptr;

	bool CameraOverride = false;
	bool RenderOverride = false;

	EngineConfig() = default;

	template <typename... ArgsT>
	void AddAction(const int STAGE, std::function<void(ArgsT...)> func, ArgsT... args) {

		if (STAGE >= DEF_STAGES) throw ConfigException(
			"Stage value given is out of bounds [0-" + std::to_string(DEF_STAGES) + "], given: " +
			std::to_string(STAGE) + "\n", 0);

		ActionStages[STAGE].AddAction(func, args...);
	}

	void SetExecOrder(const int STAGE, const std::vector<int>& order) {

		if (STAGE >= DEF_STAGES) throw ConfigException(
			"Stage value given is out of bounds [0-" + std::to_string(DEF_STAGES) + "], given: " +
			std::to_string(STAGE) + "\n", 0);

		ExecOrder[STAGE] = order;

		ExecOrderSpecified = true;

	}

	void RemoveExecOrder(const int STAGE) {

		if (STAGE >= DEF_STAGES) throw ConfigException(
			"Stage value given is out of bounds [0-" + std::to_string(DEF_STAGES) + "], given: " +
			std::to_string(STAGE) + "\n", 0);

		ExecOrder[STAGE].clear();

		ExecOrderSpecified = false;
	}

	void Exec(const int STAGE) {
		if (!ExecOrder[STAGE].empty()) {
			for (auto& i : ExecOrder[STAGE]) {
				ActionStages[STAGE].ExecAtIndex(i);
			}
		}
		else {
			ActionStages[STAGE].ExecAll();
		}
	}

};