#version 450

#define MAX_NUM_SPOT_LIGHT 4
#define MAX_NUM_JOINTS 128

layout (location = 0) in vec3 inPos;
layout (location = 1) in uvec4 inJoint0;
layout (location = 2) in vec4 inWeight0;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec2 inUV0;
layout (location = 5) in vec2 inUV1;
layout (location = 6) in vec4 inColor0;


layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;
layout (location = 4) out vec4 outColor0;
layout (location = 5) out vec4 outPosShadow[MAX_NUM_SPOT_LIGHT];


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

layout (std430, set = 3, binding = 1) readonly buffer UBONode {
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	uint jointCount;
} node;

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout(push_constant) uniform Push {
	mat4 nodeMatrix;
	int materialIndex;
} push;


void main() 
{
	
	//for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights; ++indexSpotLight) {
	//	outPosShadow[indexSpotLight] = spotLightUbo.spotLight[indexSpotLight].lightMatrix * positionWorld;
	//}

	vec4 locPos;
	if (node.jointCount > 0) {
		// Mesh is skinned
		mat4 skinMat = 
			inWeight0.x * node.jointMatrix[inJoint0.x] +
			inWeight0.y * node.jointMatrix[inJoint0.y] +
			inWeight0.z * node.jointMatrix[inJoint0.z] +
			inWeight0.w * node.jointMatrix[inJoint0.w];

		locPos = push.nodeMatrix * node.matrix * skinMat * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(push.nodeMatrix * node.matrix * skinMat))) * inNormal);
	} else {
		locPos = push.nodeMatrix * node.matrix * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(push.nodeMatrix * node.matrix))) * inNormal);
	}

	outWorldPos = locPos.xyz / locPos.w;

	outUV0 = inUV0;
	outUV1 = inUV1;
	outColor0 = inColor0;
	gl_Position =  ubo.projection * ubo.view * vec4(outWorldPos, 1);

}