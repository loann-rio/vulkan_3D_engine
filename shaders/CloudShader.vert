#version 450

#define MAX_NUM_SPOT_LIGHT 4 

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 1) out vec3 fragPosWorld;

struct PointLight {
	vec4 position;
	vec4 color;
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

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

void main() {
	mat4 model;
	mat4 normalMat;

	if (gl_InstanceIndex > 0) {
		model = push.modelMatrix * composeModelMatrix(instancePosition, instanceRotation, instanceScale);
		normalMat = push.normalMatrix * composeModelMatrix(instancePosition, instanceRotation, instanceScale);
	} 
	else
	{
		model = push.modelMatrix ;
		normalMat = push.normalMatrix;
	}

	vec4 positionWorld = model * vec4(position, 1.0); 

	gl_Position = ubo.projection * ubo.view * positionWorld;

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		fragPosShadow[indexSpotLight] = spotLightUbo.spotLight[indexSpotLight].lightMatrix * positionWorld;
	}

	fragNormalWorld = normalize(mat3(normalMat)*normal);
	fragPosWorld = positionWorld.xyz;
	fragColor = color;
	texCoord = uv;
}