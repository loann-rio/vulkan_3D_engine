#version 450

#define MAX_NUM_SPOT_LIGHT 4

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in uvec4 inJoint0;
layout (location = 5) in vec4 inWeight0;
layout (location = 6) in vec4 inColor0;


layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;
layout (location = 4) out vec4 outColor0;
layout (location = 5) out vec4 outPosShadow[MAX_NUM_SPOT_LIGHT];

#define MAX_NUM_JOINTS 128


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

/*layout (set = 0, binding = 6) uniform UBONode {
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	uint jointCount;
} node;*/

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;


void main() 
{
	
	vec4 positionWorld = push.modelMatrix * vec4(inPos, 1.0);

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights; ++indexSpotLight) {
		outPosShadow[indexSpotLight] = spotLightUbo.spotLight[indexSpotLight].lightMatrix * positionWorld;
	}

	outNormal = normalize(mat3(push.normalMatrix) * inNormal);
	outWorldPos = positionWorld.xyz;
	outUV0 = inUV0;
	outUV1 = inUV1;
	outColor0 = inColor0;

	gl_Position =  ubo.projection * ubo.view * positionWorld;
}