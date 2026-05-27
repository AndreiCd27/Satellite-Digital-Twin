#version 430 core

// Light Direction either simple SH or ZH convoluted with cosine (Funke-Hecke)
uniform vec4 LightSH[4];

// Samplers for SH visibility functions coefficients C(l,m)
uniform sampler3D V0;
uniform sampler3D V1;
uniform sampler3D V2;
uniform sampler3D V3;

uniform vec3 WorldMin;
uniform vec3 WorldMax;

uniform vec3 LightDir;

in vec4 vertPos;
in vec3 vertNormal;
in vec4 color;

out vec4 FragColor;

float GetVisibility(vec3 uvw) {

    // Extracting SH coefficients for visibility
    vec4 c0 = texture(V0, uvw);
    vec4 c1 = texture(V1, uvw);
    vec4 c2 = texture(V2, uvw);
    vec4 c3 = texture(V3, uvw);

    // Evaluate Visibility in the light's direction
    float Vis = 0.0;
    Vis += dot(c0, LightSH[0]);
    Vis += dot(c1, LightSH[1]);
    Vis += dot(c2, LightSH[2]);
    Vis += dot(c3, LightSH[3]);

    return clamp(Vis, 0.0, 1.0);
}

void main() {

    vec3 N = normalize(vertNormal);

    vec3 biasPos = vertPos.xyz - N;

    vec3 uvw;
    uvw.x = (biasPos.x - WorldMin.x) / (WorldMax.x - WorldMin.x);
    uvw.z = (biasPos.z - WorldMin.z) / (WorldMax.z - WorldMin.z); // Depth
    uvw.y = (biasPos.y - WorldMin.y) / (WorldMax.y - WorldMin.y); // Height

    uvw = clamp(uvw, 0.001, 0.999);

    float visibility = GetVisibility(uvw);

    vec3 L = normalize(LightDir);
    float lambertianTerm = max(0.2, dot(N, L));

    float ambient = 0.2;
    float directLight = lambertianTerm * visibility;
    
    vec3 finalColor = color.rgb * min(ambient + directLight, 1.0);

    FragColor = vec4(finalColor, color.a);

    //FragColor = vec4(vec3(visibility), color.a);

    //FragColor = c0;
}
