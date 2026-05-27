#version 330 core

out vec4 FragColor;

in vec4 color;
in vec3 normalVector;
in vec4 vertexShadowPosition;

uniform sampler2D shadowMap;
uniform vec3 lightDirection;

void main()
{

	vec3 projCoords = vertexShadowPosition.xyz / vertexShadowPosition.w;

	float visibility = 0.0;

	if(projCoords.z > 1.0) {
        visibility = 1.0;
    }

	float shadowFactor = texture(shadowMap, projCoords.xy).r;

	vec3 normal = normalize(normalVector);
	vec3 ToLightSource = normalize(lightDirection);

	vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
	float DOT = dot(normal,ToLightSource) + 0.9;
	vec4 diffuse = lightColor * max(DOT / 2.0, 0.0);

	vec4 ambient = vec4(0.05, 0.05, 0.05, 1.0);

	float B = 0.0; //max( 0.0001, 0.001 * (1.0 - DOT) ); 

	vec2 dtex = 1.0 / textureSize(shadowMap, 0);

	for (float xi = -2.0; xi <= 2.0; xi++) {
		for (float yi = -2.0; yi <= 2.0; yi++) {
			vec2 dxy = vec2(xi, yi) * dtex;
			shadowFactor = texture(shadowMap, projCoords.xy + dxy).r;
			visibility += ( (projCoords.z - B > shadowFactor) ? 0.5 : 1.0 );
		}
	}

	visibility /= 25.0;

    FragColor = visibility * diffuse * color + ambient;

	//FragColor = vec4(vec3(shadowFactor), 1.0); 
};