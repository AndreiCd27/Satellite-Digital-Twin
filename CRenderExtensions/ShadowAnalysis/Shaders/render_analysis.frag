#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_shadowMaskTex; 

void main() {
    float maskValue = texture(u_shadowMaskTex, TexCoords).r;
    
    FragColor = vec4(vec3(maskValue), 1.0);
}
