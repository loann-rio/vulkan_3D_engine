#version 450

#define MAX_NUM_SPOT_LIGHT 4 

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 4) in vec3 instancePosition;
layout(location = 5) in vec3 instanceRotation;
layout(location = 6) in vec3 instanceScale; 


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 texCoord;
layout(location = 4) out vec4 fragPosShadow[MAX_NUM_SPOT_LIGHT];

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

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

mat4 composeModelMatrix(vec3 translation, vec3 rotation, vec3 scale) { 
    float c3 = cos(rotation.z);
    float s3 = sin(rotation.z);
    float c2 = cos(rotation.x);
    float s2 = sin(rotation.x);
    float c1 = cos(rotation.y);
    float s1 = sin(rotation.y);

    return mat4(
        vec4(scale.x * (c1 * c3 + s1 * s2 * s3), scale.x * (c2 * s3), scale.x * (c1 * s2 * s3 - c3 * s1), 0.0),
        vec4(scale.y * (c3 * s1 * s2 - c1 * s3), scale.y * (c2 * c3), scale.y * (c1 * c3 * s2 + s1 * s3), 0.0),
        vec4(scale.z * (c2 * s1), scale.z * (-s2), scale.z * (c1 * c2), 0.0),
        vec4(translation, 1.0)
    );
}

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