#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define MAX_NUM_SPOT_LIGHT 4

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;
layout( location = 4 ) in vec4 fragPosShadow[MAX_NUM_SPOT_LIGHT]; 

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
	vec4 globalLightDir;

	vec3 camPos;

	int numLights; 

	PointLight pointLight[10]; 
} ubo;

struct ShaderMaterial {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;
	float workflow;
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;
	int occlusionTextureSet;
	int emissiveTextureSet;

	int baseColorTextureIndex;
	int metallicRoughnessTextureIndex;
	int normalTextureIndex;
	int occlusionTextureIndex;
	int emissiveTextureIndex;

	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
	float emissiveStrength;
};

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
	int materialIndex;
} push;

// every texture of the model, indexed by the material (ModelLOD::textures)
layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(std430, set = 2, binding = 1) readonly buffer SSBO {
	ShaderMaterial materials[];
};

layout(set = 1, binding = 1) uniform sampler2DShadow shadowMap[MAX_NUM_SPOT_LIGHT];

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;


vec4 compute_shadow_factor(vec4 light_space_pos, uint indexSpotLight, vec3 surfaceNormal)
{
    vec3 shadowUV = light_space_pos.xyz / light_space_pos.w;

	if (((shadowUV.x * shadowUV.x) + (shadowUV.y * shadowUV.y)) > 1.0) return vec4(0.0);

	float depth = shadowUV.z;
    shadowUV = shadowUV * 0.5 + 0.5;  // Convert to [0,1]

	float shadow = 0.0;
    float offset = 1.0 / 512.0;  // Shadow map resolution (adjust as needed)

    // Sample a 3x3 grid of shadow values
    for (int x = -1; x <= 1; x++) {
       for (int y = -1; y <= 1; y++) {
           shadow += texture(shadowMap[indexSpotLight], vec3(shadowUV.xy + vec2(x, y) * offset, depth));
       }
    }
    
	if (shadow == 0) return vec4(0.0);

	vec3 directionToLight = spotLightUbo.spotLight[indexSpotLight].position.xyz - fragPositionWorld;
	//float attenuation = 1.0 / dot(directionToLight, directionToLight);
	directionToLight = normalize(directionToLight);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);

	vec4 intencity = shadow * spotLightUbo.spotLight[indexSpotLight].color.w * 2 * vec4(spotLightUbo.spotLight[indexSpotLight].color.xyz, 0.0);// * attenuation;

	return cosAngOfIncidence * intencity / 9 ;
}

// Perturb normal with the normal map, tangent frame built from screen space derivatives
// see http://www.thetenthplanet.de/archives/1180
vec3 getNormal(ShaderMaterial material, vec3 N)
{
	vec3 tangentNormal = texture(textures[material.normalTextureIndex], fragTexCoord).xyz * 2.0 - 1.0;

	vec3 dp1 = dFdx(fragPositionWorld);
	vec3 dp2 = dFdy(fragPositionWorld);
	vec2 duv1 = dFdx(fragTexCoord);
	vec2 duv2 = dFdy(fragTexCoord);

	vec3 dp2perp = cross(dp2, N);
	vec3 dp1perp = cross(N, dp1);
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	float invmax = inversesqrt(max(dot(T, T), dot(B, B)));

	// gltf uv origin is top left, the bitangent points toward -v
	mat3 TBN = mat3(T * invmax, -B * invmax, N);

	return normalize(TBN * tangentNormal);
}

void main() {

	ShaderMaterial material = materials[push.materialIndex];

	vec3 surfaceNormal = normalize(fragNormalWorld);
	if (material.normalTextureSet > -1)
		surfaceNormal = getNormal(material, surfaceNormal);
	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraWorldPos - fragPositionWorld);

	float ao = 1.0;
	if (material.occlusionTextureSet > -1)
		ao = texture(textures[material.occlusionTextureIndex], fragTexCoord).r;

	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w * ao;
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
	vec4 color = material.baseColorFactor;
	if (material.baseColorTextureSet > -1)
		color *= texture(textures[material.baseColorTextureIndex], fragTexCoord);

	if (material.alphaMask == 1.0 && color.a < material.alphaMaskCutoff)
		discard;

	color = color * vec4(fragColor, 1.0);

	vec3 emissive = material.emissiveFactor.rgb * material.emissiveStrength;
	if (material.emissiveTextureSet > -1)
		emissive *= texture(textures[material.emissiveTextureIndex], fragTexCoord).rgb;


	// spot light mapping
	vec4 spotLightLight = {0.0, 0.0, 0.0 , 0.0};

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		spotLightLight += compute_shadow_factor(fragPosShadow[indexSpotLight], indexSpotLight, surfaceNormal);
	}

	// sum colors
	outColor = ((vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0) + cosAngOfIncidence * ubo.globalLightDir.w + spotLightLight) * color);
	outColor.rgb += emissive;

}