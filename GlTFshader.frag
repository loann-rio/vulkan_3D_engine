#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;


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
	vec4 globalLightDir;
	int numLights; 
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

// Define the texture sampler

layout(set = 1, binding = 1) uniform sampler2D colorMap;
layout(set = 1, binding = 2) uniform sampler2D physicalDescriptorMap;
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;
layout(set = 1, binding = 5) uniform sampler2D normalMap;



void main() {

	//vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 color = texture(colorMap, inUV0) * texture(aoMap, inUV0) + texture(emissiveMap, inUV0);

	// sum colors
	outColor =  color;
}