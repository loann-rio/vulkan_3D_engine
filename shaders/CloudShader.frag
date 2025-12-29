#version 450

layout( location = 0 ) in vec3 worldPos;

layout( location = 0 ) out vec4 outColor;

struct PointLight {
	vec4 position;
	vec4 color;
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

// Define the texture sampler 
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {

	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - worldPos);

	outColor = vec4(0, 0, 0, 0.5);

}