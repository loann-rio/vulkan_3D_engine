#version 450

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;

layout( location = 0 ) out vec4 outColor;

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

// Define the texture sampler
layout(set = 0, binding = 1) uniform sampler2D texSampler;


void main() {
	float depth = texture(texSampler,fragTexCoord).x;
	outColor = vec4(1.0 - (1.0 - depth) * 100.0);
}