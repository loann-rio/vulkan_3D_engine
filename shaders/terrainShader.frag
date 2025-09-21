#version 450

#define MAX_NUM_SPOT_LIGHT 4

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;
layout( location = 4 ) in vec4 fragPosShadow[MAX_NUM_SPOT_LIGHT]; 

layout( location = 0 ) out vec4 outColor;

struct PointLight {
	vec4 position;
	vec4 color;
};

struct SpotLight {
	vec4 position;
	vec4 color;
	vec4 orientation;
	mat4 lightMatrix;
};
			
layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projection;
	mat4 view;
	mat4 invView;

	vec4 ambientLightColor;
	vec4 globalLightDir;

	vec3 camPos;

	int numLights; 

	PointLight pointLight[10]; 
} ubo;

layout(set = 2, binding = 1) uniform TerrainUbo {
	float clif_slop;
	float height_grass;
	float slope_snow;
	float height_grass_with_slope;
	float height_dirt_with_slope;
	float height_snow;
} terrainUbo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

// Define the texture sampler 
layout(set = 3, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2DShadow shadowMap[MAX_NUM_SPOT_LIGHT];

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;


vec4 compute_shadow_factor(vec4 light_space_pos, uint indexSpotLight, vec3 surfaceNormal)
{
    vec3 shadowUV = light_space_pos.xyz / light_space_pos.w;

	if (((shadowUV.x * shadowUV.x) + (shadowUV.y * shadowUV.y)) > 1.0) return vec4(0.0);

	float depth = shadowUV.z;
    shadowUV = shadowUV * 0.5 + 0.5;  // Convert to [0,1]

	float shadow = 0.0;
    float offset = 1.0 / 512.0;  // Shadow map resolution (adjust as needed)

    // Sample a 3x3 grid of shadow values
    for (int x = -1; x <= 1; x++) {
       for (int y = -1; y <= 1; y++) {
           shadow += texture(shadowMap[indexSpotLight], vec3(shadowUV.xy + vec2(x, y) * offset, depth));
       }
    }
    
	if (shadow == 0) return vec4(0.0);

	vec3 directionToLight = spotLightUbo.spotLight[indexSpotLight].position.xyz - fragPositionWorld;
	//float attenuation = 1.0 / dot(directionToLight, directionToLight);
	directionToLight = normalize(directionToLight);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);

	vec4 intencity = shadow * spotLightUbo.spotLight[indexSpotLight].color.w * vec4(spotLightUbo.spotLight[indexSpotLight].color.xyz, 0.0);// * attenuation;

	return cosAngOfIncidence * intencity / 9 ;
}

void main() {

	vec3 surfaceNormal = normalize(fragNormalWorld);
	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - fragPositionWorld);

	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

	// global light

	vec3 directionToLight = normalize(ubo.globalLightDir.xyz);
		
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);
	vec3 intencity = ubo.ambientLightColor.xyz * ubo.globalLightDir.w;


	// get texture color
	vec3 color = vec3(0.0);

	// spot light mapping
	vec4 spotLightLight = {0.0, 0.0, 0.0 , 0.0};

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		spotLightLight += compute_shadow_factor(fragPosShadow[indexSpotLight], indexSpotLight, surfaceNormal);
	}

	float slope = abs(normalize(fragNormalWorld).y);
	float height = -fragPositionWorld.y;

	color = vec3(0, 0, 1);


	if (slope > terrainUbo.clif_slop) {
		// flat terrain
		color = vec3(0.1, 0.7, 0.2 );

		if (height < terrainUbo.height_grass)
		{
			// grass
			color = vec3(0.1, 0.7, 0.2 );
		}
		else
		{
			// dirt
			color = vec3( 0.2, 0.5, 0.1 );
		}

		if (height < terrainUbo.height_grass_with_slope && slope < terrainUbo.slope_snow)
		{
			// grass
			color = vec3( 0.2, 0.7, 0.2 );
		}
		else if (height < terrainUbo.height_dirt_with_slope && slope < terrainUbo.slope_snow)
		{
			// dirt
			color = vec3( 0.2, 0.5, 0.2 );
		}
		else if (height > terrainUbo.height_snow)
		{
			// snow
			color = vec3( 0.78, 0.78, 0.9 );
		}
	} 
	else 
	{
		// rock cliff
		color = vec3( 0.25, 0.239, 0.219 );
	}


	// sum colors
	outColor = (vec4(diffuseLight, 1.0) +  cosAngOfIncidence * ubo.globalLightDir.w + spotLightLight) * vec4(color, 255);
}