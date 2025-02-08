#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 texCoord;
layout(location = 4) out vec4 fragPosShadow;

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


void main() {
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

	gl_Position = ubo.projection * ubo.view * positionWorld;
	fragPosShadow = ubo.spotLight.lightMatrix * positionWorld;

	fragNormalWorld = normalize(mat3(push.normalMatrix)*normal);
	fragPosWorld = positionWorld.xyz;

	fragColor = color;

	texCoord = uv;
}