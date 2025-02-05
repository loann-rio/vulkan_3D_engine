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
	SpotLight spotLight;
	vec4 globalLightDir;
	int numLights; 
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

// Define the texture sampler
layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;

float compute_shadow_factor(vec4 light_space_pos)
{

    vec3 shadowUV = light_space_pos.xyz / light_space_pos.w;

	if (((shadowUV.x * shadowUV.x) + (shadowUV.y * shadowUV.y)) > 1.0) return 0.0;

	float depth = shadowUV.z;

    shadowUV = shadowUV * 0.5 + 0.5;  // Convert to [0,1]

	float shadow = 0.0;
    float offset = 1.0 / 512.0;  // Shadow map resolution (adjust as needed)

    // Sample a 3x3 grid of shadow values
    for (int x = -1; x <= 1; x++) {
       for (int y = -1; y <= 1; y++) {
           shadow += texture(shadowMap, vec3(shadowUV.xy + vec2(x, y) * offset, depth));
       }
    }
    
	
    return shadow / 9;

}  


void main() {

	vec3 surfaceNormal = normalize(fragNormalWorld);
	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - fragPositionWorld);

	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);

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

	vec3 directionToLight = normalize(ubo.globalLightDir.xyz);

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

	
	float isShadowed = compute_shadow_factor(fragPosShadow);
	
	// sum colors
	vec4 colorWithoutShadow = ((vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0) + cosAngOfIncidence * ubo.globalLightDir.w + (isShadowed * ubo.spotLight.color.w * vec4(ubo.spotLight.color.xyz, 0.0))) * color);
	outColor = colorWithoutShadow;// * (isShadowed * 0.9 * ubo.spotLight.color.w * vec4(ubo.spotLight.color.xyz, 0.0) + 0.1) ;  
}