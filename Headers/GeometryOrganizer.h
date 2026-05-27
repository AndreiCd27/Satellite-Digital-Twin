#pragma once

#include "GeometryLoader.h"

class Scene;
class Tile;

#define INSTANCE_ORGANIZER_TARGET 0
#define VBO_ORGANIZER_TARGET 1
#define EBO_ORGANIZER_TARGET 2

class SceneException : public std::runtime_error {
public:
	SceneException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {
		std::cerr << "[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg;
	};
};

class Scene {
private:

	// Handles are generated for every Blueprint and for every Tile
	ArrayOrganizer<InstanceData> InstanceOrganizer;

	// Handles are generated for every Tile
	ArrayOrganizer<AVertex> VBO_Organizer;
	ArrayOrganizer<GLuint> EBO_Organizer;

	// Clasify Blueprints
	std::vector<Blueprint*> Blueprints;
	std::vector<Instance*> Instances;

	std::unordered_map<std::string, int> Alias_TO_ID;
	std::unordered_map<int, Blueprint*> ID_TO_Blueprint;

	// Sample Buffer to use when selecting instances (store their index)
	// Updates every frame
	std::vector<int> VisibleInstances;

public:

	Tile* WorldRoot = nullptr;

	Scene();
	~Scene();

	Tile* FindTileForPosition(AVertex center, AVector3 Position);

	Blueprint* CreateBlueprint(std::vector<AVertex>& vertices, std::vector<GLuint>& indicies);
	Instance* CreateInstance(Blueprint* temp, AVector3 pos);

	ArrayOrganizer<InstanceData>& GetInstanceOrganizer();
	ArrayOrganizer<AVertex>& GetVBO_Organizer();
	ArrayOrganizer<GLuint>& GetEBO_Organizer();

	std::vector<Blueprint*>& GetBlueprints();
	std::vector<Instance*>& GetInstances();

	const InstanceData* GetInstanceOrganizerPTR(int HandleID);
	const AVertex * GetVBO_OrganizerPTR(int HandleID);
	const GLuint * GetEBO_OrganizerPTR(int HandleID);
	
	/*
	std::pair<const InstanceData*, int> GetInstanceOrganizerRange(int HandleID_0, int HandleID_1);
	std::pair<const AVertex*, int> GetVBO_OrganizerRange(int HandleID_0, int HandleID_1);
	std::pair<const GLuint*, int> GetEBO_OrganizerRange(int HandleID_0, int HandleID_1);
	*/

	Handle GetBlueprintHandle(Blueprint* BLUEPRINT, int TARGET);

	void GenerateHandle(int HandleID, int TARGET, int capacity);
};

class Tile {
private:
	static int contor;

	int TileID;

	Tile* Parent;
	uint16_t TileX, TileZ;
	uint16_t Level = 0;

	Tile* Divisions[2][2] = { nullptr };

	std::vector<int> RelatedHandleIDs;

public:
	static const int shiftComponent;

	Tile(Tile* _Parent, uint16_t _TileX, uint16_t _TileZ, uint16_t _Level);
	~Tile();
	void DivideTile(uint16_t i, uint16_t j);
	uint16_t GetLevel() { return Level; }

	std::vector<Tile*> RecurseInTiles();
	void RecurseInTilesOutputHandleIDs(std::vector<int>& HandleIDs);

	const int GetTileID() const;

	const std::vector<int>& GetRelatedHandleIDs();
	void PushHandleID(int HandleID, ArrayOrganizer<InstanceData>& insArrayOrg);

	friend Tile* Scene::FindTileForPosition(AVertex center, AVector3 Position);
};