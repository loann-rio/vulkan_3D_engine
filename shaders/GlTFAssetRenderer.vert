#version 450

#define MAX_NUM_SPOT_LIGHT 4
#define MAX_NUM_JOINTS 64

layout (location = 0) in vec3 inPos;
layout (location = 1) in uvec4 inJoint0;
layout (location = 2) in vec4 inWeight0;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec2 inUV0;
layout (location = 5) in vec2 inUV1;
layout (location = 6) in vec4 inColor0;

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

layout (std430, set = 3, binding = 0) readonly buffer UBONode {
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
	int none;
} push;


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


void main() 
{

	mat4 modelMat = push.nodeMatrix ;

	vec4 locPos;
	if (node.jointCount > 0) {
		// Mesh is skinned
		mat4 skinMat = 
			inWeight0.x * node.jointMatrix[inJoint0.x] + 
			inWeight0.y * node.jointMatrix[inJoint0.y] +
			inWeight0.z * node.jointMatrix[inJoint0.z] +
			inWeight0.w * node.jointMatrix[inJoint0.w];

		locPos = modelMat * node.matrix * skinMat * vec4(inPos, 1.0);
		fragNormalWorld = normalize(transpose(inverse(mat3(modelMat * node.matrix * skinMat))) * inNormal);
	} else {
		locPos = modelMat * node.matrix * vec4(inPos, 1.0);
		fragNormalWorld = normalize(transpose(inverse(mat3(modelMat * node.matrix))) * inNormal);
	}

	fragPosWorld = locPos.xyz / locPos.w;

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights; ++indexSpotLight) {
		fragPosShadow[indexSpotLight] = spotLightUbo.spotLight[indexSpotLight].lightMatrix * vec4(fragPosWorld, 1); 
	}

	texCoord = inUV0;
	fragColor = inColor0.xyz;
	gl_Position =  ubo.projection * ubo.view * vec4(fragPosWorld, 1);
}