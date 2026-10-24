#version 450

#define MAX_NUM_SPOT_LIGHT 4
#define MaxIterations 100


layout( location = 0 ) in vec2 uv;

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
	
// Define the texture sampler 
layout(set = 0, binding = 1) uniform sampler2D texSampler;

void main() {
    //outColor = vec4(texCoord.xy,0,1);

    outColor = texture(texSampler, uv);
}
