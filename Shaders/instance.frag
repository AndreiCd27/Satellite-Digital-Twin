#version 430 core

out vec4 FragColor;

in vec4 color;
in vec3 normalVector;
in vec4 vertexShadowPosition;

uniform sampler2D shadowMap;
uniform vec3 lightDirection;

void main()
{

	vec3 projCoords = vertexShadowPosition.xyz / vertexShadowPosition.w;

	if (projCoords.x < 0.0 || projCoords.y < 0.0 || projCoords.x > 1.0 || projCoords.y > 1.0 || projCoords.z > 1.0) {
		FragColor = color;
		return;
	}

	float visibility = 0.0;

	float shadowFactor = 0.0f;

	//vec3 normal = normalize(normalVector);
	vec3 normal = normalize(cross(dFdx(vertexShadowPosition.xyz), dFdy(vertexShadowPosition.xyz)));
	vec3 ToLightSource = normalize(lightDirection);
	float DOT = dot(normal,ToLightSource);
	float B = max( 0.0005 * (1.0 - DOT), 0.00005 ); 

	vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
	float diffuse = min(max(DOT, 0.0), 1.0);

	vec4 ambient = vec4(0.25, 0.25, 0.25, 1.0);

	vec2 dtex = 1.0 / textureSize(shadowMap, 0);

	for (float xi = -1.0; xi <= 1.0; xi++) {
		for (float yi = -1.0; yi <= 1.0; yi++) {
			vec2 dxy = vec2(xi, yi) * dtex;
			shadowFactor = texture(shadowMap, projCoords.xy + dxy).r;
			visibility += ( (projCoords.z - B > shadowFactor) ? 0.125 : 1.0 );
		}
	}

	visibility /= 9.0;

	float lightFactor = clamp(visibility * diffuse * 1.25 + ambient.r, 0.0, 1.0);
    FragColor = vec4(lightFactor * color.xyz, 1.0);

	//FragColor = vec4(vec3(shadowFactor), 1.0); 
};