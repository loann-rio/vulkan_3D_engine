#version 450

#define MAX_NUM_SPOT_LIGHT 4 

layout(location = 0) in vec3 position;

struct SpotLight {
	vec4 position;
	vec4 color;
	vec4 orientation;
	mat4 lightMatrix;
};
	

struct ObjectData { mat4 model; };
layout(std430, set = 0, binding = 1) readonly buffer Objects { ObjectData objects[]; };

layout(push_constant) uniform Push {
	uint matrixIndex;
	uint lightIndex;
} push;


layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

void main() {
	vec4 positionWorld = objects[push.matrixIndex].model * vec4(position, 1.0);
	gl_Position = spotLightUbo.spotLight[push.lightIndex].lightMatrix * positionWorld; 
}