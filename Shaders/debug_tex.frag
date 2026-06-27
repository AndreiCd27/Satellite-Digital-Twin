#version 430 core

in vec2 vertUV;
out vec4 FragColor;

uniform sampler2D debug_tex;

uniform float scale = 1.0;
uniform int channels = 3;

void main() {
    
    vec3 clrMask = vec3(1.0);
    if (channels == 1) {
        clrMask = vec3(1.0,0.0,0.0);
    }
    if (channels == 2) {
        clrMask = vec3(1.0,1.0,0.0);
    }

    vec3 val = texture(debug_tex, vec2(vertUV.x * 2, 1.0) - vertUV).rgb;

    vec3 vizVal = val * clrMask * scale;

    if (scale < 1.0) {
        // May have negative values, implicit normalization
        vizVal = (vizVal + vec3(1.0)*clrMask) * 0.5;
    }

    FragColor = vec4(vizVal, 1.0);
}
