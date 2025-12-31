#version 450

layout( location = 0 ) in vec3 worldPos;
layout( location = 0 ) out vec4 outColor;

struct PointLight {
	vec4 position;
	vec4 color;
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

// Define the texture sampler 
layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform CloudUbo {
	vec4 min_rect;
	vec4 max_rect;
	float baseAlpha;
	float noiseSizeFactor;
	float stepSize;
	float noiseFactorpreexp;
	float noiseFactorpostexp;
	float fadeDistance;
	
	int numStep;


} cloudUbo;

vec3 random3(vec3 p)
{
    return fract(sin(vec3(
        dot(p, vec3(127.1, 311.7,  74.7)),
        dot(p, vec3(269.5, 183.3, 246.1)),
        dot(p, vec3(113.5, 271.9, 124.6))
    )) * 43758.5453);
}

float random3f(vec3 p)
{
    return fract(sin(
        dot(p, vec3(127.1, 311.7,  74.7))
    ) * 43758.5453);
}

bool insideBox(vec3 p, vec3 bmin, vec3 bmax)
{
    return all(greaterThanEqual(p, bmin)) &&
           all(lessThanEqual(p, bmax));
}

bool intersectAABB(
    vec3 ro, vec3 rd,
    vec3 bmin, vec3 bmax,
    out float tEnter,
    out float tExit)
{
    vec3 invDir = 1.0 / rd;

    vec3 t0 = (bmin - ro) * invDir;
    vec3 t1 = (bmax - ro) * invDir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    tEnter = max(max(tmin.x, tmin.y), tmin.z);
    tExit  = min(min(tmax.x, tmax.y), tmax.z);

    return tExit >= max(tEnter, 0.0);
}



float voronoi(vec3 pos) {
	vec3 i_st = floor(pos/cloudUbo.noiseSizeFactor);
    vec3 f_st = fract(pos/cloudUbo.noiseSizeFactor);

	float m_dist = 1.;

	for (int z= -1; z <= 1; z++) {
		for (int y= -1; y <= 1; y++) {
			for (int x= -1; x <= 1; x++) {
				// Neighbor place in the grid
				vec3 neighbor = vec3(float(x),float(y), float(z));

				// Random position from current + neighbor place in the grid
				vec3 point = random3(i_st + neighbor);

				// Animate the point
				point = 0.5 + 0.5 * sin(6.2831 * point);

				// Vector between the pixel and the point
				vec3 diff = neighbor + point - f_st;

				// Distance to the point
				float dist = length(diff);

				// Keep the closer distance
				m_dist = min(m_dist, dist);
			}
		}
	}

	return m_dist;
}

float boxFade(vec3 p, vec3 bmin, vec3 bmax)
{
    vec3 d = min(p - bmin, bmax - p);
    float edge = min(d.x, min(d.y, d.z));
    return smoothstep(0.0, cloudUbo.fadeDistance, edge);
}


void main() {

	vec3 cameraWorldPos = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(worldPos - cameraWorldPos);

	float tEnter;
    float tExit;

    if (!intersectAABB(
        cameraWorldPos,
        viewDirection,
        cloudUbo.min_rect.xyz,
        cloudUbo.max_rect.xyz,
        tEnter,
        tExit))
    {
        discard;
    }


	float L = tExit - tEnter;
	float stepSize = L / float(cloudUbo.numStep);

    vec3 p = cameraWorldPos + viewDirection * tEnter;
	float jitter = random3f(worldPos) * stepSize;
	float t = tEnter + jitter;

	float transmittance = 1.0;
	
	int i;
	for (i = 0; i < cloudUbo.numStep && t < tExit; ++i)
    {
		float fade = boxFade(p, cloudUbo.min_rect.xyz, cloudUbo.max_rect.xyz);
        float d = voronoi(p) * stepSize * fade;
		float extinction = d * cloudUbo.noiseFactorpreexp;
		transmittance *= exp(-extinction * stepSize);
		
		t += stepSize;
        p = cameraWorldPos + viewDirection * t;
    }

	float alpha = transmittance * cloudUbo.noiseFactorpostexp;

	outColor = vec4(1, 1, 1, clamp(exp(- alpha), 0.0, 1.0));

}