#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputColor;

void main() {
    outColor = vec4(texture(inputColor, uv).xyz, 1.0);
}