#version 430 core
layout (location = 0) in vec3 APosition;

uniform mat4 lightPerspMatrix;

void main()
{
	gl_Position = lightPerspMatrix * vec4(APosition, 1.0);
};