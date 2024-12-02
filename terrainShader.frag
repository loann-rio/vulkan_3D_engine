#version 450

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;

layout( location = 0 ) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

struct PointLight {
	vec4 position;
	vec4 color;
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



void main() {

	vec3 diffuseLight = ubo.ambientLightColor.xyz * max(ubo.ambientLightColor.w, max(dot(fragNormalWorld, -ubo.globalLightDir.xyz), 0) * ubo.globalLightDir.w);
	vec3 specularLight = vec3(0.0);


	vec3 surfaceNormal = normalize(fragNormalWorld);


	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - fragPositionWorld);

	

	vec4 color = texture(texSampler, fragTexCoord) * fragTexCoord.x * fragTexCoord.y;

	// water level
	//float height = -fragPositionWorld.y*2/3;

	/*if (color.w > 0.7){
		color = vec4(0.137, 0.192, 0.596, 1.0);
	} else if (color.w <  0.4){
		color = vec4(0.145, 0.271, 0.659, 1.0 );
	} */

	

	outColor = vec4(color.xyz, 1.0);
	// sum colors
	//outColor = (vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0)) * color;// + vec4(5.0, 5.0, 5.0, 1.0);//+  vec4(globalLight, 1.0);  
}