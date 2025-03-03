#version 450

#define MAX_NUM_SPOT_LIGHT 4

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) in vec4 inPosShadow[MAX_NUM_SPOT_LIGHT]; 


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
	PointLight pointLight[10];
	vec4 globalLightDir;
	int numLights; 
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

// shadow layout
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMap[MAX_NUM_SPOT_LIGHT];

layout(set = 2, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

// Define the texture sampler
layout(set = 1, binding = 1) uniform sampler2D colorMap;
layout(set = 1, binding = 2) uniform sampler2D physicalDescriptorMap;
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;
layout(set = 1, binding = 5) uniform sampler2D normalMap;



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

	vec3 directionToLight = spotLightUbo.spotLight[indexSpotLight].position.xyz - inWorldPos;
	//float attenuation = 1.0 / dot(directionToLight, directionToLight);
	directionToLight = normalize(directionToLight);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);

	vec4 intencity = shadow * spotLightUbo.spotLight[indexSpotLight].color.w * vec4(spotLightUbo.spotLight[indexSpotLight].color.xyz, 0.0);// * attenuation;

	return cosAngOfIncidence * intencity / 9 ;
}



void main() {

	vec3 surfaceNormal = normalize(inNormal);
	vec3 directionToLight = normalize(ubo.globalLightDir.xyz);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);
	vec3 intencity = ubo.ambientLightColor.xyz * ubo.globalLightDir.w;

	vec4 spotLightLight = {0.0, 0.0, 0.0 , 0.0};

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		spotLightLight += compute_shadow_factor(inPosShadow[indexSpotLight], indexSpotLight, surfaceNormal);
	}

	//vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 color = (texture(colorMap, inUV0) * texture(aoMap, inUV0)) * (cosAngOfIncidence * ubo.globalLightDir.w + spotLightLight) + texture(emissiveMap, inUV0); 

	// sum colors
	outColor =  color;
}