#version 450

#define MAX_NUM_SPOT_LIGHT 4
#define MAX_NUM_JOINTS 64

struct SpotLight {
	vec4 position;
	vec4 color;
	vec4 orientation;
	mat4 lightMatrix;
};

layout(set = 1, binding = 0) uniform SpotLightUbo {
	SpotLight spotLight[MAX_NUM_SPOT_LIGHT];
	int numLights;
} spotLightUbo;

layout(push_constant) uniform Push {
        mat4 modelMatrix;
        int lightIndex;
        uint nodeIndex;
        uint jointOffset;
        uint jointCount;
} push;

layout(std430, set = 3, binding = 0) readonly buffer Pose {
        mat4 m[];
} pose;

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

layout(location = 0) in vec3 inPos;
layout(location = 1) in uvec4 inJoint0;
layout(location = 2) in vec4 inWeight0;

layout(location = 3) in vec3 instancePosition;
layout(location = 4) in vec3 instanceRotation;
layout(location = 5) in vec3 instanceScale; 

void main()
{
	 mat4 model;
    if (gl_InstanceIndex > 0) {
            model = push.modelMatrix * composeModelMatrix(instancePosition, instanceRotation, instanceScale);
    } else {
            model = push.modelMatrix;
    }

    mat4 local;
    if (push.jointCount > 0) {
            local = inWeight0.x * pose.m[push.jointOffset + inJoint0.x]
                    + inWeight0.y * pose.m[push.jointOffset + inJoint0.y]
                    + inWeight0.z * pose.m[push.jointOffset + inJoint0.z]
                    + inWeight0.w * pose.m[push.jointOffset + inJoint0.w];
    } else {
            local = pose.m[push.nodeIndex];
    }

    vec4 worldPos = model * local * vec4(inPos, 1.0);
    gl_Position = spotLightUbo.spotLight[push.lightIndex].lightMatrix * worldPos;
}  