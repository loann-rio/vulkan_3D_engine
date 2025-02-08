#version 450

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

layout(location = 0) in vec3 position;
 
void main()
{
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
	gl_Position = ubo.spotLight.lightMatrix * positionWorld;
}