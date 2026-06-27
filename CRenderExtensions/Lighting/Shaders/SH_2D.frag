#version 430 core

uniform vec4 LightSH[4];

layout(binding = 0) uniform sampler2D V0;
layout(binding = 1) uniform sampler2D V1;
layout(binding = 2) uniform sampler2D V2;
layout(binding = 3) uniform sampler2D V3;

layout(binding = 4) uniform sampler2D UnifiedHeightmap;
layout(binding = 5) uniform sampler2D SatelliteTex;

uniform vec3 LightDir;

// UI Slider Uniforms
uniform float DirectShadowStrength = 0.85;
uniform float AmbientShadowContrast = 1.0;
uniform float AmbientShadowIntensity = 0.7;

uniform float DirectLightIntensity = 1.0; // Exposure parameter for sun light
uniform float AmbientLightIntensity = 0.3; // Exposure parameter for sky/indirect light


uniform float DNI;
uniform float DHI;
uniform float GHI;

uniform float MetersPerPixel = 0.3;

in vec4 vertPos;
in vec3 vertNormal;
in vec4 color;
in vec2 mapUV;
in float sfactor;

out vec4 FragColor;

float GetVisibility2D(vec2 uv) {
    
    vec4 c0 = texture(V0, uv);
    vec4 c1 = texture(V1, uv);
    vec4 c2 = texture(V2, uv);
    vec4 c3 = texture(V3, uv);

    float Vis = 0.0;
    Vis += dot(c0, LightSH[0]);
    Vis += dot(c1, LightSH[1]);
    Vis += dot(c2, LightSH[2]);
    Vis += dot(c3, LightSH[3]);
    
    return clamp(Vis, 0.0, 1.0);
}

bool IsInShadowRaymarched(vec3 startPosNorm, vec2 size) {
    vec3 worldStep = normalize(LightDir) * MetersPerPixel;
    vec3 texStep = vec3(worldStep.x / size.x, worldStep.y, worldStep.z / size.y);
    
    if (length(texStep) < 0.0001) {
        return false; 
    }

    vec3 N = normalize(vertNormal);
    vec3 L = normalize(LightDir);
    float dotNL = max(0.0, dot(N, L));

    float wallBias = mix(0.5, 0.01, dotNL); 
    
    // Push the ray origin out along the wall normal and slightly up/forward
    vec3 biasedStart = startPosNorm;
    biasedStart.xz += (N.xz * 0.002); // Push away from the wall texture plane
    biasedStart.y  += wallBias; // Push above the local step artifact profile

    vec3 samplePos = biasedStart + texStep * 1.5;

    for (int step = 1; step <= 120; step += 2) {
        if (samplePos.x < 0.0 || samplePos.x > 1.0 || samplePos.z < 0.0 || samplePos.z > 1.0) {
            break;
        }
        
        float currentHeight = texture(UnifiedHeightmap, samplePos.xz).r + texture(UnifiedHeightmap, samplePos.xz).g;

        // Check against the heightmap value
        if (currentHeight > samplePos.y) {
            return true;
        }

        samplePos += texStep * 2.0;
    }
    return false;
}

void main() {
    vec3 N = normalize(vertNormal);
    vec2 hmapSize = vec2(textureSize(UnifiedHeightmap, 0));

    vec3 L = normalize(LightDir);
    float lambert = max(0.0, dot(N, L));

    // Get current height in world meters
    float startHeight = texture(UnifiedHeightmap, mapUV).r + texture(UnifiedHeightmap, mapUV).g;
    vec3 startPos = vec3(mapUV.x, startHeight, mapUV.y);
    //vec3 startPos = vec3(mapUV.x, vertPos.y, mapUV.y);

    // Calculate shadow via raymarching
    float shadowFactor = 1.0;
    if (lambert > 0.0) {
        shadowFactor = IsInShadowRaymarched(startPos, hmapSize) ? 0.0 : 1.0;
    } else {
        shadowFactor = 0.0;
    }

    shadowFactor = mix(1.0, shadowFactor, DirectShadowStrength);

    float ambientVis = GetVisibility2D(mapUV);
    ambientVis = pow(ambientVis, AmbientShadowContrast);
    ambientVis = mix(1.0, ambientVis, AmbientShadowIntensity);

    vec4 texColor = texture(SatelliteTex, mapUV);
    vec3 baseColor = color.rgb * texColor.rgb;

    vec3 directLighting = baseColor * (lambert * shadowFactor * DirectLightIntensity);
    vec3 ambientLighting = baseColor * (ambientVis * AmbientLightIntensity);

    // Final color accumulation
    FragColor = vec4(directLighting + ambientLighting, 1.0);
}