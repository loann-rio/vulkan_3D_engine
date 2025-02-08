#version 450

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

#define MAX_NUM_JOINTS 128


layout (set = 0, binding = 6) uniform UBONode {
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	uint jointCount;
} node;

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
	SpotLight spotLight[3];
	vec4 globalLightDir;
	int numLights; 
} ubo;


layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;


void main() 
{
	outColor0 = inColor0;

	vec4 locPos = push.modelMatrix * vec4(inPos, 1.0);
	outNormal = normalize(vec3(1.0));

	locPos.y = -locPos.y;
	outWorldPos = locPos.xyz / locPos.w;
	outUV0 = inUV0;
	outUV1 = inUV1;
	gl_Position =  ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}