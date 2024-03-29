#version 450

layout( location = 0 ) in vec3 fragColor;
layout( location = 1 ) in vec3 fragPositionWorld;
layout( location = 2 ) in vec3 fragNormalWorld;
layout( location = 3 ) in vec2 fragTexCoord;

layout( location = 0 ) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

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
	//vec4 color = texture(texSampler, fragTexCoord);
	vec4 color = vec4(fragColor, 1.0);

	/*
	std::vector<terrainType> regions{
        {.3f  , {.137f, .192f, .596f }},
        {.4f  , {.145f, .271f, .659f }},
        {.45f , {   }},
        {.55f , {.145f, .659f, .188f }},
        {.6f  , { }},
        {.7f  , { }},
        {.85f , {}},
        {1.f  , {1.f  , 1.f  , 1.f   }},
    };
	*/   

	float height = -fragPositionWorld.y;
	if (height <  0.15){
		color = vec4(0.137, 0.192, 0.596, 1.0);
	} else if (height <  0.23){ //128){
		color = vec4(0.145, 0.271, 0.659, 1.0 );
	} /*else if (height <  0.1822){
		color = vec4(0.878, 0.823, 0.4, 1.0);
	} else if (height < 0.33275 ){
		color = (vec4(fragColor, 1.0) + vec4(0.145, 0.6590, 0.188, 1.0))/2;
	} else if (height <  0.432){
		color = (vec4(fragColor, 1.0) + vec4(0.176, 0.557, 0.208, 1.0))/2;
	} else if (height < 0.686 ){
		color = (vec4(fragColor, 1.0) + vec4(0.482, 0.376, 0.247, 1.0))/2;  
	} else if (height < 1.22825 ){
		color = (vec4(fragColor, 1.0) + vec4(0.443, 0.322, 0.212, 1.0 ))/2;
	} else {
		color = (vec4(fragColor, 1.0) + vec4(1.0, 1.0, 1.0, 1.0))/2;
	}*/


	// sum colors
	outColor = (vec4(diffuseLight, 1.0) + vec4(specularLight, 1.0)) * color +  cosAngOfIncidence * ubo.globalLightDir.w * color;  
}