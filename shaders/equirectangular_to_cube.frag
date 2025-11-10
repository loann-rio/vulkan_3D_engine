#version 450

layout(binding = 1) uniform sampler2D equirectMap;

layout(push_constant) uniform Push {
    mat4 view;   // view for the current face (rot)
    mat4 proj;   // 90deg proj, but for equirect->cube we only need direction
} push;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

const vec2 invAtan = vec2(0.1591, 0.3183098861837907); // 1/(2*pi), 1/pi constants combo?

vec2 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    // reconstruct direction in world space by unprojecting screen -> world
    // we render a fullscreen triangle where vertex shader provided uv, but
    // we can compute direction by using view inverse * direction.
    // Simpler: compute a direction from gl_FragCoord via normalized device coords:
    vec2 uv_in = uv;
    uv_in.x = 1 - uv_in.x;
    vec2 ndc = uv_in * 2.0 - 1.0;
    // We need a direction for the cube face: use push.view inverse to transform
    vec4 dir = inverse(push.view) * vec4(normalize(vec3(ndc.x, ndc.y, 1.0)), 0.0);
    vec3 direction = normalize(dir.xyz);

    vec2 equirectUV = sampleSphericalMap(direction);
    vec3 color = texture(equirectMap, equirectUV).rgb;
    outColor = vec4(color, 1.0);
}