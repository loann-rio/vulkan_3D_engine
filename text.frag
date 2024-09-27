#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) out vec4 outFragColor;

void main(void)
{
	float color = texture(samplerFont, inUV).r;

	if (color > 0.1) {
		outFragColor = vec4(0, 0, 0, 255);
		}
		else {
		outFragColor = vec4(0, 0, 0, 0);
		}
	
}