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

layout(location = 0) in vec3 inPos;
layout(location = 1) in uvec4 inJoint0;
layout(location = 2) in vec4 inWeight0;

layout(location = 3) in vec3 instancePosition;
layout(location = 4) in vec3 instanceRotation;
layout(location = 5) in vec3 instanceScale; 

void main()
{
	mat4 model;
	if (gl_InstanceIndex > 0) {
		model = push.nodeMatrix * composeModelMatrix(instancePosition, instanceRotation, instanceScale);
	} 
	else
	{
		model = push.nodeMatrix ;
	}
	
	vec4 locPos;
	if (node.jointCount > 0) {
		mat4 skinMat = 
			inWeight0.x * node.jointMatrix[inJoint0.x] +
			inWeight0.y * node.jointMatrix[inJoint0.y] +
			inWeight0.z * node.jointMatrix[inJoint0.z] +
			inWeight0.w * node.jointMatrix[inJoint0.w];

		locPos = model * node.matrix * skinMat * vec4(inPos, 1.0); 
	} else {
		locPos = model * node.matrix * vec4(inPos, 1.0); 
	}
			
	vec3 worldPos = locPos.xyz / locPos.w;

	gl_Position = spotLightUbo.spotLight[push.lightIndex].lightMatrix * vec4(worldPos, 1.0);  
}  