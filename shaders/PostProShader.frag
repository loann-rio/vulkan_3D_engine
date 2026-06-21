#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

struct PointLight {
       vec4 position;
       vec4 color;
};

layout(push_constant) uniform Push {
    mat4 view;   
    mat4 proj; 
} push;


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

layout(set = 1, binding = 0) uniform sampler2D inputColor;

void main() {
    outColor = vec4(texture(inputColor, uv).xyz, 1.0);
    //outColor = vec4(uv, 1.0, 1.0);
}