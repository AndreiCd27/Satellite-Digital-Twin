#pragma once


#include "precompile.h"

#include "GeometryBasics.h"
#include "MultiArray.h"
#include "Camera.h"
#include "VBO.h"
#include "EBO.h"

#define TILE_MAXSCALE_FACTOR 14
#define MAX_TILE_LEVEL 14
#define START_TILE_LEVEL 10

class Blueprint;
class Tile;
class Instance;
class Scene;

class BlueprintException : public std::runtime_error {
public:
	BlueprintException(const std::string& Msg, int errorCode) :
		std::runtime_error("[ BLUEPRINT_ERROR ] -> ERR_" + std::to_string(errorCode) + " : " + Msg) {};
};

class Blueprint {
	static const int safeBlueprintCount; // Throws an exception if we go beyond 4095 blueprints
	// If any modifications are to be made, consider looking into the shiftComponent variable
	// In the Tile class and modify it such that no conflicts arise
	// At the moment, the shift component is calculated as follows: (int) log2l(Blueprint::safeBlueprintCount) + 1;

	static int contor; // Used to set templateID, simple incremented variable, starting at 0

	// Stores the vertices and indicies of the blueprint, any instance of this blueprint
	// Has this same geometry, but with different Position, Rotation and Scale
	int VerticesHandleID;  // FROM templateID TO vertices_buffer in Scene
	int IndiciesHandleID;  // FROM templateID TO indicies_buffer in Scene
	// Toghether, they form the geometry that define this object

	int templateID; // TemplateID is used in combination with TileID to form a HandleID
	// HandleID will be the key to access data shared between tiles and blueprints 
	// (like transformation matricies)
	// Only 4095 Blueprints may be created, the rest 20 bits are reserved for tileIDs
	// A simple OR (|) operation can generate the key we need for Blueprint A in Tile B 
	// ( HandleID = A | ( B << 12 ) )

	const char* Alias;

	Blueprint();

public:

	int GetID();

	AVertex Center;
	static const int GetShiftComponent();

	void SetColor(int R, int G, int B, int A);

	static void CalculateSurfaceNormals(std::vector<AVertex>& worldVert, std::vector<GLuint>& indicies);

	friend class Scene;
};

// ABSTRACT CLASS ENTITY
class Entity {
	static int contor;
	int EID;
	std::string TagName = "";
	bool Active = true;
protected:
	Scene* ParentScene = nullptr;
public:
	Entity(Scene* scene, std::string _TagName);
	virtual ~Entity() = default;

	virtual void Update() = 0;
};


// ABSTRACT CLASS TRANSFORM
class Transform : public Entity {
	void Defaults();
protected:
	AVector3 Rotation = AVector3(0.0f, 0.0f, 0.0f); // Rotation in degrees for X,Y,Z axis
	AVector3 Size = AVector3(1.0f, 1.0f, 1.0f); // Scale Vector for X,Y,Z axis
	AVector3 Position = AVector3(0.0f, 0.0f, 0.0f); // Translate Vector for X,Y,Z axis
	AColor3 Color;
public:
	Transform(Scene* scene);
	Transform(Scene* scene, std::string TagName);
	virtual ~Transform() = default;
};

class Instance : public Transform {

	Blueprint* Template = nullptr; // Blueprint object
	//Add more stuff, like materials and textures
	Tile* tile = nullptr; // Tile it belongs in, calculated through Position
	int handleOffset = 0; 
	// The offset calculated from the coresponding HandleID

	void Update() override;

public:
	Instance(Blueprint* _Template, Scene* scene, std::string TagName);
	Instance(Blueprint* _Template, Scene* scene);

	void SetPosition(AVector3 _Position);
	void SetRotation(AVector3 _Rotation);
	void SetSize(AVector3 _Size);
	void SetColor(int R, int G, int B, int A);
	void SetColor(AColor3 _Color);
	void SetTile(Tile* _tile);

	int GetBlueprintID();

	AVector3 GetPosition();
	AVector3 GetRotation();
	AVector3 GetSize();

	void SetHandleOffset(int offset);

};

class InstanceData {
private:
	glm::mat4 matrix = glm::mat4(1.0f);
	AColor3 RGBA; // 4 bytes
	A_UV UV; // 4 bytes
public:
	InstanceData() = default;
	InstanceData(AVector3 Position, AVector3 Rotation, AVector3 Scale);
	void SetMatrix(AVector3 Position, AVector3 Rotation, AVector3 Scale);
	void SetColor(AColor3 _RGBA);
};

class UniqueGeom : public Transform {
	// Geometry remains unchanged, stored directly as geometry
};

class StaticGeom : public Entity {

};
