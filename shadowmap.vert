#version 450

#define MAX_NUM_SPOT_LIGHT 4

struct SpotLight {
	vec4 position;
	vec4 color;
	vec4 orientation;
	mat4 lightMatrix;
};

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	int lightIndex;
} push;

layout(set = 2, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout(location = 0) in vec3 position;
 
void main()
{
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
	gl_Position = spotLightUbo.spotLight[push.lightIndex].lightMatrix * positionWorld;
} 