#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

struct PointLight {
	vec4 position;
	vec4 color;
};


layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

	
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

layout(location = 0) out vec3 texDir;

void main() {
    texDir = inPos;
    mat4 rotView = mat4(mat3(ubo.view)); // remove translation
    vec4 pos = ubo.projection * rotView * vec4(inPos, 1.0);
    gl_Position = pos.xyww; // force depth = 1
}
