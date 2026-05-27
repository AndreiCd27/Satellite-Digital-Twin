#pragma once


#include "precompile.h"

struct AColor3123 {
	float r, g, b;
};


struct AVertex2132 { //48 bytes
	float lx = 0.0f;
	float hx = 0.0f;
	float lz = 0.0f;
	float hz = 0.0f;
	float y = 0.0f;
	float r, g, b;
	float nx = 0.0f, ny = 0.0f, nz = 0.0f; //normal vector
	float meshID = 0.0f;
};

class A_UV {
public:
	uint32_t UV = 0;
	A_UV() = default;
	A_UV(float U, float V);
};

class AColor3 {
public:
	uint32_t RGBA = 0;
	AColor3() = default;
	AColor3(int R, int G, int B, int A);
};

// Could have used glm
class AVector3 {
public:
	float x = 0.0f, y = 0.0f, z = 0.0f;
	AVector3() = default;
	AVector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};
	~AVector3() = default;
	AVector3 operator+(const AVector3& dr);
	AVector3 operator*(const AVector3& dr);
	AVector3 operator*(const float& scalar);
	AVector3& operator+=(const AVector3& dr);
	AVector3 operator^(const AVector3& dr); // Used for cross product
	AVector3 Normalize();

	operator glm::vec3() const;
};

class AVertex {
public:
	//--------------- 0 bytes
	AVector3 POS;
	AColor3 RGBA;
	//--------------- 16 bytes
	AVector3 NORMAL;
	A_UV UV;
	//--------------- 32 bytes
	AVertex() = default;
	AVertex(float x, float y, float z);
	AVertex(float x, float y, float z, int R, int G, int B, int A);
	AVertex(AVector3 _POS, AVector3 _NORMAL, int R, int G, int B, int A, float U, float V);
};
