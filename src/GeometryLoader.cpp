

#include "GeometryLoader.h"

int Entity::contor = 0;

Entity::Entity(Scene* scene, std::string _TagName) : TagName(_TagName), Active(false) {
	ParentScene = scene;
	EID = contor;
	contor++;
}

void Transform::Defaults() {
	Rotation = AVector3(0.0f, 0.0f, 0.0f);
	Size = AVector3(1.0f, 1.0f, 1.0f);
	Position = AVector3(0.0f, 0.0f, 0.0f);
	Color = AColor3( 200, 200, 200, 255 );
}

Transform::Transform(Scene* scene) : Entity(scene, "Transform") {
	Defaults();
}
Transform::Transform(Scene* scene, std::string TagName) : Entity(scene, TagName) {
	Defaults();
}

Instance::Instance(Blueprint* Template, Scene* scene) : Transform(scene, "Instance") {
	this->Template = Template;
}
Instance::Instance(Blueprint* Template, Scene* scene, std::string TagName) : Transform(scene, TagName) {
	this->Template = Template;
}

void Instance::SetPosition(AVector3 _Position) {
	Position = _Position;
	Update();
}
void Instance::SetRotation(AVector3 _Rotation) {
	Rotation = _Rotation;
	Update();
}
void Instance::SetSize(AVector3 _Size) {
	Size = _Size;
	Update();
}
void Instance::SetColor(AColor3 _Color) {
	Color = _Color;
	Update();
}
void Instance::SetColor(int R, int G, int B, int A) {
	Color = AColor3(R, G, B, A);
	Update();
}
void Instance::SetTile(Tile* _tile) {
	tile = _tile;
}

int Instance::GetBlueprintID() { return Template->GetID(); }

AVector3 Instance::GetPosition() { return Position; }
AVector3 Instance::GetRotation() { return Rotation; }
AVector3 Instance::GetSize() { return Size; }

void Instance::SetHandleOffset(int offset) { handleOffset = offset; }


void InstanceData::SetMatrix(AVector3 Position, AVector3 Rotation, AVector3 Scale) {
	matrix = glm::translate(glm::mat4(1.0f), (glm::vec3)Position);
	matrix = glm::rotate(matrix, glm::radians(Rotation.x), glm::vec3(1,0,0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.y), glm::vec3(0,1,0));
	matrix = glm::rotate(matrix, glm::radians(Rotation.z), glm::vec3(0,0,1));
	matrix = glm::scale(matrix, (glm::vec3)Scale);
}
void InstanceData::SetColor(AColor3 _RGBA) {
	RGBA = _RGBA;
}

InstanceData::InstanceData(AVector3 Position, AVector3 Rotation, AVector3 Scale) {
	SetMatrix(Position, Rotation, Scale);
}

int Blueprint::contor = 0;
const int Blueprint::safeBlueprintCount = 4095;

const int Blueprint::GetShiftComponent() {
	return (int) log2l(Blueprint::safeBlueprintCount) + 1;
}

Blueprint::Blueprint() {

	if (contor >= safeBlueprintCount) {
		throw BlueprintException(" Blueprint limit exceded! Maximum Blueprint Count: " + std::to_string(safeBlueprintCount), 0);
	}

	templateID = contor;
	contor++;
}

void Blueprint::SetColor(int R, int G, int B, int A) {
	Center.RGBA = AColor3(R, G, B, A);
}

int Blueprint::GetID() { return templateID; }

void assignNormal(AVertex& v, glm::vec3 N) {
	// Add to already existing normal (if to vertex has been assigned a normal before, initial normal = {0.0f})
	v.NORMAL.x += N.x; v.NORMAL.y += N.y; v.NORMAL.z += N.z;
}

void Blueprint::CalculateSurfaceNormals(std::vector<AVertex>& worldVert, std::vector<GLuint>& indicies) {
	for (int i = 0; i < indicies.size(); i++) {
		AVertex& v = worldVert[indicies[i]];
		v.NORMAL = AVector3(0.0f, 0.0f, 0.0f);
	}
	for (int i = 0; i < (indicies.size() / 3) * 3; i += 3) {
		// We calculate the normal of that triangle face and forward them to every vertex of that face
		AVertex& A = worldVert[indicies[i]];
		AVertex& B = worldVert[indicies[i + 1]];
		AVertex& C = worldVert[indicies[i + 2]];
		glm::vec3 vectAB = (glm::vec3)B.POS - (glm::vec3)A.POS;
		glm::vec3 vectAC = (glm::vec3)C.POS - (glm::vec3)A.POS;
		glm::vec3 cross = glm::cross(vectAB, vectAC);
		assignNormal(A, cross);
		assignNormal(B, cross);
		assignNormal(C, cross);
	}
	for (int i = 0; i < indicies.size(); i++) {
		AVertex& v = worldVert[indicies[i]];
		v.NORMAL = v.NORMAL.Normalize();
	}
}
