#include "ModelAnimation.h"
#include "ModelNode.h" 

#include <algorithm>
#include <glm/fwd.hpp>

namespace {

	float normalizedTime(float time, float t0, float t1)
	{
		return std::max((time - t0) / (t1 - t0), 0.0f);
	}

	glm::quat vec4ToQuat(const glm::vec4& v)
	{
		return glm::quat(v.w, v.x, v.y, v.z);
	}

}


// Cube spline interpolation function used for translate/scale/rotate with cubic spline animation samples
// Details on how this works can be found in the specs https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
glm::vec4 AnimationSampler::cubicSplineInterpolation(size_t index, float time, uint32_t stride)
{
	float delta = inputs[index + 1] - inputs[index];
	float t = (time - inputs[index]) / delta;
	const size_t current = index * stride * 3;
	const size_t next = (index + 1) * stride * 3;
	const size_t A = 0;
	const size_t V = stride * 1;
	const size_t B = stride * 2;

	float t2 = powf(t, 2);
	float t3 = powf(t, 3);
	glm::vec4 pt{ 0.0f };
	for (uint32_t i = 0; i < stride; i++) {
		float p0 = outputs[current + i + V];			// starting point at t = 0
		float m0 = delta * outputs[current + i + A];	// scaled starting tangent at t = 0
		float p1 = outputs[next + i + V];				// ending point at t = 1
		float m1 = delta * outputs[next + i + B];		// scaled ending tangent at t = 1
		pt[i] = ((2.f * t3 - 3.f * t2 + 1.f) * p0) + ((t3 - 2.f * t2 + t) * m0) + ((-2.f * t3 + 3.f * t2) * p1) + ((t3 - t2) * m1);
	}
	return pt;

}

void AnimationSampler::translate(size_t index, float time, Node* node)
{
	switch (interpolation) {
		case AnimationSampler::InterpolationType::LINEAR: {
			const float u = normalizedTime(time, inputs[index], inputs[index + 1]);
			node->transform.translation = 
				glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
			break;
		}

		case AnimationSampler::InterpolationType::STEP: {
			node->transform.translation = outputsVec4[index];
			break;
		}

		case AnimationSampler::InterpolationType::CUBICSPLINE: {
			node->transform.translation = cubicSplineInterpolation(index, time, 3);
			break;
		}
	}
}


void AnimationSampler::scale(size_t index, float time, Node* node)
{
	switch (interpolation) {
	case AnimationSampler::InterpolationType::LINEAR: {
		const float u = normalizedTime(time, inputs[index], inputs[index + 1]);
		node->transform.scale = 
			glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
		break;
	}

	case AnimationSampler::InterpolationType::STEP: {
		node->transform.scale = outputsVec4[index];
		break;
	}

	case AnimationSampler::InterpolationType::CUBICSPLINE: {
		node->transform.scale = cubicSplineInterpolation(index, time, 3);
		break;
	}
	}

}

void AnimationSampler::rotate(size_t index, float time, Node* node)
{
	switch (interpolation) {
	case AnimationSampler::InterpolationType::LINEAR: {
		const float u = normalizedTime(time, inputs[index], inputs[index + 1]);

		const glm::quat q0 = vec4ToQuat(outputsVec4[index]);
		const glm::quat q1 = vec4ToQuat(outputsVec4[index + 1]);

		node->transform.rotation = glm::normalize(glm::slerp(q0, q1, u));
		break;
	}
	case AnimationSampler::InterpolationType::STEP: {
		
		node->transform.rotation = 
			vec4ToQuat(outputsVec4[index]);
		break;
	}
	case AnimationSampler::InterpolationType::CUBICSPLINE: {
		const glm::vec4 rot = cubicSplineInterpolation(index, time, 4);
		
		node->transform.rotation = 
			glm::normalize(vec4ToQuat(rot));
		break;
	}
	}

}