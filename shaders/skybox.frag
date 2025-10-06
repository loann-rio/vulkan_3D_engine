#version 450
layout(location = 0) in vec3 texDir;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform samplerCube skybox;

void main() {
    outColor = texture(skybox, normalize(texDir));
}
