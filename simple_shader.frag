#version 450

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;
layout( location = 4 ) in vec4 fragPosShadow;

layout( location = 0 ) out vec4 outColor;

struct PointLight {
	vec4 position;
	vec4 color;
};
			
layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projection;
	mat4 view;
	mat4 invView;

	mat4 lightProjection;
	mat4 lightView;

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
layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler2D depthSampler;

float compute_shadow_factor(vec4 light_space_pos, sampler2D shadow_map)
{
   // Convert light space position to NDC
   vec3 light_space_ndc = light_space_pos.xyz /= light_space_pos.w;
 
   // If the fragment is outside the light's projection then it is outside
   // the light's influence, which means it is in the shadow 
   if (abs(light_space_ndc.x) > 1.0 ||
       abs(light_space_ndc.y) > 1.0 ||
       abs(light_space_ndc.z) > 1.0)
      return 0.0;

   if ((light_space_ndc.x * light_space_ndc.x) + (light_space_ndc.y * light_space_ndc.y) > 1.0)
	return 0.0;
 
   // Translate from NDC to shadow map space
   vec2 shadow_map_coord = light_space_ndc.xy * 0.5 + 0.5;
 
   // Check if the sample is in the light or in the shadow
   if (light_space_ndc.z > texture(shadow_map, shadow_map_coord.xy).x)
      return 0.0; // In the shadow
 
   // In the light
   return 1.0;
}  


void main() {

	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);


	vec3 surfaceNormal = normalize(fragNormalWorld);


	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - fragPositionWorld);

	// apply points light
	for (int i = 0; i < ubo.numLights; i++) 
	{

		PointLight light = ubo.pointLight[i];

		vec3 directionToLight = light.position.xyz - fragPositionWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);

		float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intencity = light.color.xyz * light.color.w * attenuation;

		diffuseLight += intencity * cosAngOfIncidence;

		// specular lighting
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 32.0);
		specularLight += intencity * blinnTerm;

	}

	// global light

	//vec4 globalLightDir; -> 4th value = intencity
	//vec4 ambientLightColor; 

	vec3 directionToLight = ubo.globalLightDir.xyz;

	directionToLight = normalize(directionToLight);

	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);
	vec3 intencity = ubo.ambientLightColor.xyz * ubo.globalLightDir.w;

	// specular lighting
	/*vec3 halfAngle = normalize(directionToLight + viewDirection);
	float blinnTerm = dot(surfaceNormal, halfAngle);
	blinnTerm = clamp(blinnTerm, 0, 1);
	blinnTerm = pow(blinnTerm, 32.0);
	specularLight += intencity * blinnTerm;*/


	// get texture color
	vec4 color = texture(texSampler, fragTexCoord);
	//vec4 color = vec4(fragColor, 1.0);
	//vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

	// sum colors

	float isShadowed = compute_shadow_factor(fragPosShadow, depthSampler);

	outColor = ((vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0)) * color +  cosAngOfIncidence * ubo.globalLightDir.w * color) * (isShadowed * 0.9 + 0.1);  
}