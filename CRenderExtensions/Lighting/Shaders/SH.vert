#version 430 core
layout (location = 0) in vec3 APosition;
layout (location = 1) in vec4 AColor;
layout (location = 2) in vec3 ANormal;
layout (location = 3) in uint AUV;

uniform mat4 perspectiveMatrix;
uniform vec3 CamPosition;

uniform vec3 WorldMin;
uniform vec3 WorldSInverse; // 1.0 / (WorldMax - WorldMin)

out vec4 color;
out vec4 vertPos;
out vec3 vertNormal;
out vec2 mapUV;
out float sfactor;

void main()
{   
    
    vec4 WorldPos = vec4(APosition, 1.0);
    vec3 RelPos = WorldPos.xyz - CamPosition;
    gl_Position = perspectiveMatrix * vec4(RelPos, 1.0);

    vec2 maphUV;
    maphUV.x = (APosition.x - WorldMin.x) * WorldSInverse.x;
    maphUV.y = (APosition.z - WorldMin.z) * WorldSInverse.z;
    maphUV = clamp(maphUV, 0.001, 0.999);

    vertPos = WorldPos;
    mapUV = vec2(maphUV.x, 1.0 - maphUV.y);;
    vertNormal = ANormal;
    color = vec4(1.0);

    sfactor = 0.25;
    if (AUV == 1) {
        sfactor = 1.0;
    }
    
}