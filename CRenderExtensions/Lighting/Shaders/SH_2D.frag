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

uniform float DirectLightIntensity = 1.0;
uniform float AmbientLightIntensity = 0.3;

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

bool IsInShadowRaymarched(vec3 startPosNorm, vec2 size, vec3 N) {
    vec3 worldStep = normalize(LightDir) * MetersPerPixel;
    vec3 texStep = vec3(worldStep.x / size.x, worldStep.y, worldStep.z / size.y);
    
    if (length(texStep) < 0.0001) {
        return false;
    }

    vec3 L = normalize(LightDir);
    float dotNL = max(0.0, dot(N, L));

    float wallBias = mix(0.5, 0.01, dotNL);
    
    vec3 biasedStart = startPosNorm;
    biasedStart.xz += (N.xz * 0.002);
    biasedStart.y  += wallBias;

    vec3 samplePos = biasedStart + texStep * 1.5;

    for (int step = 1; step <= 120; step += 2) {
        if (samplePos.x < 0.0 || samplePos.x > 1.0 || samplePos.z < 0.0 || samplePos.z > 1.0) {
            break;
        }
        
        float buildingHeight = texture(UnifiedHeightmap, samplePos.xz).r;
        float terrainHeight = texture(UnifiedHeightmap, samplePos.xz).g;
        float currentHeight = buildingHeight + terrainHeight;

        if (currentHeight - 0.5 > samplePos.y && buildingHeight > 0.5) {
            return true;
        }

        samplePos += texStep * 2.0;
    }
    return false;
}

void main() {
    vec2 hmapSize = vec2(textureSize(UnifiedHeightmap, 0));
    vec2 texelSize = 1.0 / hmapSize;

    // Sample 5x5 window for Wall Detection & Normal Computation
    float maxBuildingHeightInWindow = 0.0;
    float currentTerrainHeight = texture(UnifiedHeightmap, mapUV).g;

    // Gradient Accumulators
    float gradX = 0.0;
    float gradZ = 0.0;

    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) {
            vec2 offsetUV = mapUV + vec2(float(x), float(z)) * texelSize;
            float bHeight = texture(UnifiedHeightmap, offsetUV).r;
            
            // Max Height in 5x5 window
            if (bHeight > maxBuildingHeightInWindow) {
                maxBuildingHeightInWindow = bHeight;
            }

            // Gradient filter (Central Difference extended to 5x5 window)
            gradX += bHeight * float(x);
            gradZ += bHeight * float(z);
        }
    }

    float buildingEdgeWorldHeight = maxBuildingHeightInWindow + currentTerrainHeight;
    bool isBuildingWall = (vertPos.y < (buildingEdgeWorldHeight - 0.4)) && (maxBuildingHeightInWindow > 0.5);

    vec3 N;
    if (isBuildingWall) {
        // Gradiant vector for Normal Vector calculation
        gradX *= MetersPerPixel;
        gradZ *= MetersPerPixel;

        N = normalize(vec3(-gradX, 0.0, -gradZ) + vec3(0.00001, 0.0, 0.00001));
    } else {
        // Implicit Normal for building roofs or terrain
        N = normalize(cross(dFdx(vertPos.xyz), dFdy(vertPos.xyz)));
    }

    vec3 L = normalize(LightDir);
    float lambert = max(0.0, dot(N, L));

    vec3 startPos = vec3(mapUV.x, vertPos.y, mapUV.y);

    vec4 texColor = texture(SatelliteTex, mapUV);
    vec3 baseColor = color.rgb * (1.0 - sfactor) + texColor.rgb * sfactor;

    float shadowFactor = 1.0;

    if (isBuildingWall) {
        shadowFactor = 1.0;
    } else {
        if (lambert > 0.0) {
            shadowFactor = IsInShadowRaymarched(startPos, hmapSize, N) ? 0.0 : 1.0;
        } else {
            shadowFactor = 0.0;
        }
        shadowFactor = mix(1.0, shadowFactor, DirectShadowStrength);
    }

    float ambientVis = GetVisibility2D(mapUV);
    ambientVis = pow(ambientVis, AmbientShadowContrast);
    ambientVis = mix(1.0, ambientVis, AmbientShadowIntensity);
    ambientVis = ambientVis * AmbientLightIntensity;

    float directLighting = lambert * shadowFactor * DirectLightIntensity;
    FragColor = vec4(baseColor * min(directLighting + ambientVis, 1.0), 1.0);
}
