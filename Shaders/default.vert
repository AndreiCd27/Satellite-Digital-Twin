#version 330 core
layout (location = 0) in vec3 APosition;
layout (location = 1) in vec4 AColor;
layout (location = 2) in vec3 ANormal;
layout (location = 3) in vec2 AUV;

uniform mat4 perspectiveMatrix;
uniform vec3 CamPosition;

uniform mat4 lightPerspMatrix;

out vec4 color;
out vec3 normalVector;
out vec4 vertexShadowPosition;

void main()
{
	gl_Position = perspectiveMatrix * vec4(APosition - CamPosition, 1.0);

	vertexShadowPosition = lightPerspMatrix * vec4(APosition, 1.0);

	color = AColor;

	normalVector = ANormal;
};