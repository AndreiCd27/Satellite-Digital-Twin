#version 430 core

out vec2 vertUV;

void main() {
    // ID 0: (-1, -1), ID 1: (3, -1), ID 2: (-1, 3)
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    
    vertUV.x = (x + 1.0) * 0.5;
    vertUV.y = (y + 1.0) * 0.5;
    
    gl_Position = vec4(x, y, 0.0, 1.0);
}
