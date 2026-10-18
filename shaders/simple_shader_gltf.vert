#version 450

#define MAX_NUM_SPOT_LIGHT 4 

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv0;
layout(location = 4) in vec2 uv1;
layout(location = 5) in uvec4 joint0;
layout(location = 6) in vec4 weight0; 

layout(location = 7) in vec3 instancePosition;
layout(location = 8) in vec3 instanceRotation;
layout(location = 9) in vec3 instanceScale; 


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 texCoord;
layout(location = 4) out vec4 fragPosShadow[MAX_NUM_SPOT_LIGHT];

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
        mat4 modelMatrix;
        int materialIndex;
        uint nodeIndex;
        uint jointOffset;
        uint jointCount;
} push;


layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout(std430, set = 3, binding = 0) readonly buffer Pose {
        mat4 m[];
} pose;

mat4 localMatrix() {
        if (push.jointCount == 0) return pose.m[push.nodeIndex];

        return weight0.x * pose.m[push.jointOffset + joint0.x]
             + weight0.y * pose.m[push.jointOffset + joint0.y]
             + weight0.z * pose.m[push.jointOffset + joint0.z]
             + weight0.w * pose.m[push.jointOffset + joint0.w];
}

mat4 composeModelMatrix(vec3 translation, vec3 rotation, vec3 scale) { 
    float c3 = cos(rotation.z);
    float s3 = sin(rotation.z);
    float c2 = cos(rotation.x);
    float s2 = sin(rotation.x);
    float c1 = cos(rotation.y);
    float s1 = sin(rotation.y);

    return mat4(
        vec4(scale.x * (c1 * c3 + s1 * s2 * s3), scale.x * (c2 * s3), scale.x * (c1 * s2 * s3 - c3 * s1), 0.0),
        vec4(scale.y * (c3 * s1 * s2 - c1 * s3), scale.y * (c2 * c3), scale.y * (c1 * c3 * s2 + s1 * s3), 0.0),
        vec4(scale.z * (c2 * s1), scale.z * (-s2), scale.z * (c1 * c2), 0.0),
        vec4(translation, 1.0)
    );
}

void main() {
	mat4 model;

	if (gl_InstanceIndex > 0) {
		model = push.modelMatrix * composeModelMatrix(instancePosition, instanceRotation, instanceScale);
	} 
	else
	{
		model = push.modelMatrix ;
	}
     
    mat4 local = localMatrix(); 
    mat4 world = model * local;
    vec4 positionWorld = world * vec4(position, 1.0);

    for (uint indexSpotLight = 0; indexSpotLight < spotLightUbo.numLights && indexSpotLight < MAX_NUM_SPOT_LIGHT; ++indexSpotLight) {
            fragPosShadow[indexSpotLight] = spotLightUbo.spotLight[indexSpotLight].lightMatrix * positionWorld;
    }

    gl_Position = ubo.projection * ubo.view * positionWorld;
    fragNormalWorld = normalize(transpose(inverse(mat3(world))) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
    texCoord = uv0;
}