
#include "GeometryBasics.h"

AColor3::AColor3(int R, int G, int B, int A) {
	RGBA = ((A & 255) << 24) | ((B & 255) << 16) | ((G & 255) << 8) | (R & 255);
	// | Red 0-255 | Green 0-255 | Blue 0-255 | Aplha 0-255 |
	//    4 bytes  +   4 bytes   +   4 bytes  +   4 bytes
}
const float uvConst = 65536.0f;
A_UV::A_UV(float U, float V) {
	uint16_t _U = (uint16_t)(U * uvConst);
	uint16_t _V = (uint16_t)(V * uvConst);
	UV = ((uint32_t)_U << 16) | _V;
}
A_UV::A_UV(uint32_t UV) : UV(UV) {}

AVector3 AVector3::operator+(const AVector3& dr) const {
	return { dr.x + this->x, dr.y + this->y, dr.z + this->z };
}
AVector3& AVector3::operator+=(const AVector3& dr) {
	this->x += dr.x; this->y += dr.y; this->z += dr.z;
	return *this;
}
AVector3 AVector3::operator-(const AVector3& dr) const {
	return { this->x - dr.x, this->y - dr.y, this->z - dr.z };
}
AVector3& AVector3::operator-=(const AVector3& dr) {
	this->x -= dr.x; this->y -= dr.y; this->z -= dr.z;
	return *this;
}
AVector3 AVector3::operator*(const AVector3& dr) const {
	return { dr.x * this->x, dr.y * this->y, dr.z * this->z };
}
AVector3 AVector3::operator*(const float& scalar) const {
	return { scalar * this->x, scalar * this->y, scalar * this->z };
}
AVector3 AVector3::operator^(const AVector3& dr) const { //CROSS PRODUCT
	return {
		this->y * dr.z - this->z * dr.y,
		this->z * dr.x - this->x * dr.z,
		this->x * dr.y - this->y * dr.x
	};
}
AVector3& AVector3::operator=(const glm::vec3& dr) {
	this->x = dr.x;
	this->y = dr.y;
	this->z = dr.z;
	return *this;
}
AVector3 AVector3::Normalize() {
	float dist = std::sqrt(x * x + y * y + z * z);
	if (dist < 0.000001f) return AVector3( 0.00001f, 0.00001f, 0.00001f );
	return AVector3( x / dist, y / dist, z / dist );
}
void AVector3::Normalize_InPlace() {
	float dist = std::sqrt(x * x + y * y + z * z);
	if (dist < 0.000001f) { *this = AVector3(0.0f, 0.0f, 0.0f); return; }
	x /= dist; y /= dist; z /= dist;
}
AVector3 AVector3::Rotate(const AVector3& ROT) {

	glm::quat quatX = glm::angleAxis(glm::radians(ROT.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quatY = glm::angleAxis(glm::radians(ROT.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quatZ = glm::angleAxis(glm::radians(ROT.z), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::vec3 t = quatX * quatY * quatZ * (glm::vec3)*this;

	return AVector3(t.x, t.y, t.z);
}
void AVector3::Rotate_InPlace(const AVector3& ROT) {
	// Quaternion for Rotation (easier computation)
	glm::quat quatX = glm::angleAxis(glm::radians(ROT.x), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat quatY = glm::angleAxis(glm::radians(ROT.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat quatZ = glm::angleAxis(glm::radians(ROT.z), glm::vec3(0.0f, 0.0f, 1.0f));

	*this = quatX * quatY * quatZ * (glm::vec3)*this;
}

float AVector3::Dot(const AVector3 v) const {
	return x * v.x + y * v.y + z * v.z;
}

AVector3::operator glm::vec3() const {
	return glm::vec3(x, y, z);
}

float AVector3::Magnitude() const {
	return sqrt(x * x + y * y + z * z);
}

void AVector3::DEBUG_Print() const {
	std::cout << "(" << this->x << ", " << this->y << ", " << this->z << "), ";
}


AVertex::AVertex(float x, float y, float z) : POS(x,y,z), NORMAL(AVector3(0.0f, 1.0f, 0.0f)) {}

AVertex::AVertex(float x, float y, float z, int R, int G, int B, int A) 
	: POS(x,y,z), RGBA(R, G, B, A), NORMAL(AVector3(0.0f, 1.0f, 0.0f)) {}

AVertex::AVertex(AVector3 _POS, AVector3 _NORMAL, int R, int G, int B, int A, float U, float V) : 
	POS(_POS), RGBA(R, G, B, A), NORMAL(_NORMAL), UV(U, V) {}

AVertex::AVertex(float x, float y, float z, uint8_t R, uint8_t G, uint8_t B, uint8_t A, uint32_t UV) :
	POS(x,y,z), RGBA(R, G, B, A), NORMAL(AVector3(0.0f, 1.0f, 0.0f)), UV(UV) { }