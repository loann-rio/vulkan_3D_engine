#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

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
	vec4 globalLightDir;
	int numLights; 
} ubo;

layout(push_constant) uniform Push {
	vec4 position;
	vec4 color;
	float radius;
} push;

const float M_PI = 3.14159265358;
void main() {
	float dist = dot(fragOffset, fragOffset);
	if (dist >= 1.0) { discard; }
	outColor = vec4(push.color.xyz, 0.5 * (cos(dist * M_PI) + 1));
}