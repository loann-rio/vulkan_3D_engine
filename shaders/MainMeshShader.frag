#version 450

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec2 fragTexCoord;

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

	vec4 diffuseLight = vec4(ubo.ambientLightColor.xyz * ubo.ambientLightColor.w, 1);
	vec4 textureColor = texture(texSampler, fragTexCoord);
	vec4 fragColor    = vec4(fragColor, 1);

	if (textureColor.a < 0.9)
		discard;

	// sum colors
	outColor = (diffuseLight * textureColor * fragColor);

}