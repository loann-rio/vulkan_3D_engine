#version 450

#define MAX_NUM_SPOT_LIGHT 4
#define MAX_NUM_JOINTS 64

struct SpotLight {
	vec4 position;
	vec4 color;
	vec4 orientation;
	mat4 lightMatrix;
};

layout(push_constant) uniform Push {
	mat4 nodeMatrix;
	int lightIndex;
} push;

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout (std430, set = 3, binding = 0) readonly buffer UBONode {
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	uint jointCount;
} node;

layout (location = 0) in vec3 inPos;
layout (location = 1) in uvec4 inJoint0;
layout (location = 2) in vec4 inWeight0;

void main()
{
	vec4 locPos;
	if (node.jointCount > 0) {
		mat4 skinMat = 
			inWeight0.x * node.jointMatrix[inJoint0.x] +
			inWeight0.y * node.jointMatrix[inJoint0.y] +
			inWeight0.z * node.jointMatrix[inJoint0.z] +
			inWeight0.w * node.jointMatrix[inJoint0.w];

		locPos = push.nodeMatrix * node.matrix * skinMat * vec4(inPos, 1.0); 
	} else {
		locPos = push.nodeMatrix * node.matrix * vec4(inPos, 1.0); 
	}
			
	vec3 worldPos = locPos.xyz / locPos.w;

	gl_Position = spotLightUbo.spotLight[push.lightIndex].lightMatrix * vec4(worldPos, 1.0);  
}  