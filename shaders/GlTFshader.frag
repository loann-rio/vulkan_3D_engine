#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define MAX_NUM_SPOT_LIGHT 4

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) in vec4 inPosShadow[MAX_NUM_SPOT_LIGHT]; 

layout( location = 0 ) out vec4 outColor;

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

struct PBRInfo 
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};


const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;
const float PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1.0;



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

layout(push_constant) uniform Push {
	
	mat4 nodeMatrix;
	int materialIndex;
} push;

// shadow layout
layout(set = 1, binding = 1) uniform sampler2DShadow shadowMap[MAX_NUM_SPOT_LIGHT];

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;



layout(std430, set = 2, binding = 2) readonly buffer SSBO
{
   ShaderMaterial materials[ ];
};

layout(set = 2, binding = 1) uniform sampler2D textures[];


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
           shadow += texture(shadowMap[nonuniformEXT(indexSpotLight)], vec3(shadowUV.xy + vec2(x, y) * offset, depth));
       }
    }
    
	if (shadow == 0) return vec4(0.0);

	vec3 directionToLight = spotLightUbo.spotLight[indexSpotLight].position.xyz - inWorldPos;
	//float attenuation = 1.0 / dot(directionToLight, directionToLight);
	directionToLight = normalize(directionToLight);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);

	vec4 intencity = shadow * spotLightUbo.spotLight[indexSpotLight].color.w * vec4(spotLightUbo.spotLight[indexSpotLight].color.xyz, 0.0);// * attenuation;

	return cosAngOfIncidence * intencity / 9 ;
}

vec3 getNormal(ShaderMaterial material)
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(textures[nonuniformEXT(materials[0].normalTextureIndex)], material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

vec3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / M_PI;
}

vec3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
}

// Gets metallic factor from specular glossiness workflow inputs 
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < c_MinRoughness) {
		return 0.0;
	}
	float a = c_MinRoughness;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;
	float c = c_MinRoughness - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
	#define MANUAL_SRGB 1
	#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
	#else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
	vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
	#endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut,srgbIn.w);;
	#else //MANUAL_SRGB
	return srgbIn;
	#endif //MANUAL_SRGB
}



void main() {


	//vec4 color = (texture(textures[nonuniformEXT(materials[push.materialIndex].baseColorTextureIndex)], inUV0)); 
	//outColor =  color;

	vec3 surfaceNormal = normalize(inNormal);
	/*vec3 directionToLight = normalize(ubo.globalLightDir.xyz);
	float cosAngOfIncidence = max(dot(surfaceNormal, directionToLight), 0);
	vec3 intencity = ubo.ambientLightColor.xyz * ubo.globalLightDir.w;

	vec4 spotLightLight = {0.0, 0.0, 0.0 , 0.0};

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		spotLightLight += compute_shadow_factor(inPosShadow[indexSpotLight], indexSpotLight, surfaceNormal);
	}

	vec4 color = inColor0;
	//vec4 color = (texture(colorMap, inUV0) * texture(aoMap, inUV0)) * (cosAngOfIncidence * ubo.globalLightDir.w + spotLightLight) + texture(emissiveMap, inUV0); 

	// sum colors
	outColor =  color;*/

	
	ShaderMaterial material = materials[push.materialIndex];

	float perceptualRoughness;
	float metallic;
	vec3 diffuseColor;
	vec4 baseColor;

	vec3 f0 = vec3(0.04);

	if (material.alphaMask == 1.0f) {
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLINEAR(texture(textures[nonuniformEXT(material.baseColorTextureIndex)], material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor; 
		} else {
			baseColor = material.baseColorFactor;
		}
		if (baseColor.a < material.alphaMaskCutoff) {
			discard;
		}
	}

	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) { 
		// Metallic and Roughness material properties are packed together
		// In glTF, these factors can be specified by fixed scalar values
		// or from a metallic-roughness map
		perceptualRoughness = material.roughnessFactor;
		metallic = material.metallicFactor; 
		if (material.physicalDescriptorTextureSet > -1) { 
			// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
			// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
			vec4 mrSample = texture(textures[nonuniformEXT(material.metallicRoughnessTextureIndex)], material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
			perceptualRoughness = mrSample.g * perceptualRoughness;
			metallic = mrSample.b * metallic;
		} else {
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
		// Roughness is authored as perceptual roughness; as is convention,
		// convert to material roughness by squaring the perceptual roughness [2].

		// The albedo may be defined from a base texture or a flat color
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLINEAR(texture(textures[nonuniformEXT(material.baseColorTextureIndex)], material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
	}

	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSSINESS) {
		// Values from specular glossiness workflow are converted to metallic roughness
		if (material.physicalDescriptorTextureSet > -1) {
			perceptualRoughness = 1.0 - texture(textures[nonuniformEXT(material.metallicRoughnessTextureIndex)], material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;
		} else {
			perceptualRoughness = 0.0;
		}

		const float epsilon = 1e-6;

		vec4 diffuse = SRGBtoLINEAR(texture(textures[nonuniformEXT(material.baseColorTextureIndex)], inUV0));
		vec3 specular = SRGBtoLINEAR(texture(textures[nonuniformEXT(material.metallicRoughnessTextureIndex)], inUV0)).rgb;

		float maxSpecular = max(max(specular.r, specular.g), specular.b);

		// Convert metallic value from specular glossiness inputs
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - c_MinRoughness) / max(1 - metallic, epsilon)) * material.diffuseFactor.rgb;
		vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1 - metallic) * (1 / max(metallic, epsilon))) * material.specularFactor.rgb;
		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);
	}

	baseColor *= inColor0;

	diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
		
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	vec3 specularColor = mix(f0, baseColor.rgb, metallic);

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 n = (material.normalTextureSet > -1) ? getNormal(material) : normalize(inNormal);
	n.y *= -1.0f;

	vec3 v = normalize(ubo.camPos - inWorldPos);    // Vector from surface point to camera
	vec3 l = normalize(ubo.globalLightDir.xyz);     // Vector from surface point to light
	vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 reflection = normalize(reflect(-v, n));

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	PBRInfo pbrInputs = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);

	// Calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbrInputs);
	float G = geometricOcclusion(pbrInputs);
	float D = microfacetDistribution(pbrInputs);

	const vec3 u_LightColor = vec3(1.0);

	// Calculation of analytical lighting contribution
	vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
	vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV); // NdotL

	// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 color = NdotL * u_LightColor * (specContrib + diffuseContrib); // NdotL

	const float u_OcclusionStrength = 1.0f;
	// Apply optional PBR terms for additional (optional) shading
	if (material.occlusionTextureSet > -1) {
		float ao = texture(textures[nonuniformEXT(material.occlusionTextureIndex)], (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		color = mix(color, color * ao, u_OcclusionStrength);
	}

	vec3 emissive = material.emissiveFactor.rgb * material.emissiveStrength;
	if (material.emissiveTextureSet > -1) {
		emissive *= SRGBtoLINEAR(texture(textures[nonuniformEXT(material.emissiveTextureIndex)], material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb;
	};


	// apply spotlight shadow map
	vec4 spotLightLight = {0.0, 0.0, 0.0 , 0.0};

	for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
		spotLightLight += compute_shadow_factor(inPosShadow[indexSpotLight], indexSpotLight, surfaceNormal);
	}

	
	color += ubo.ambientLightColor.w * ubo.ambientLightColor.rgb * diffuseColor;
	color += emissive;
	
	outColor = spotLightLight * vec4(color, baseColor.a);
}