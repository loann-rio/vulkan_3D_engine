
#include "Animation.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

    glm::quat toQuat(const glm::vec4& v) { return glm::quat(v.w, v.x, v.y, v.z); }

    // index i such that inputs[i] <= t <= inputs[i + 1]
    size_t findKey(const std::vector<float>& inputs, float t) {
        auto it = std::upper_bound(inputs.begin(), inputs.end(), t);
        if (it == inputs.begin()) return 0;
        return std::min(static_cast<size_t>(it - inputs.begin()) - 1, inputs.size() - 2);
    }

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#appendix-c-interpolation
    glm::vec4 cubicSpline(const AnimationSampler& s, size_t i, float u, float delta) {
        const glm::vec4 p0 = s.outputs[i * 3 + 1];
        const glm::vec4 m0 = delta * s.outputs[i * 3 + 2];    
        const glm::vec4 p1 = s.outputs[(i + 1) * 3 + 1];
        const glm::vec4 m1 = delta * s.outputs[(i + 1) * 3 + 0]; 

        const float u2 = u * u;
        const float u3 = u2 * u;
        return (2.f * u3 - 3.f * u2 + 1.f) * p0 + (u3 - 2.f * u2 + u) * m0
            + (-2.f * u3 + 3.f * u2) * p1 + (u3 - u2) * m1;
    }

    void applyChannel(const AnimationChannel& channel, const AnimationSampler& s, float time, NodePose& pose) {
        const bool cubic = s.interpolation == AnimationSampler::CUBICSPLINE;
        const size_t keyCount = s.inputs.size();
        if (keyCount == 0 || s.outputs.size() < keyCount * (cubic ? 3 : 1)) return;

        glm::vec4 value;
        if (keyCount == 1 || time <= s.inputs.front()) {
            value = s.outputs[cubic ? 1 : 0];
        }
        else if (time >= s.inputs.back()) {
            value = s.outputs[cubic ? (keyCount - 1) * 3 + 1 : keyCount - 1];
        }
        else {
            const size_t i = findKey(s.inputs, time);
            const float delta = s.inputs[i + 1] - s.inputs[i];
            const float u = delta > 0.f ? (time - s.inputs[i]) / delta : 0.f;

            switch (s.interpolation) {
            case AnimationSampler::STEP:
                value = s.outputs[i];
                break;
            case AnimationSampler::LINEAR:
                if (channel.path == AnimationChannel::ROTATION) {
                    glm::quat q = glm::normalize(glm::slerp(toQuat(s.outputs[i]), toQuat(s.outputs[i + 1]), u));
                    pose.rotation = q;
                    return;
                }
                value = glm::mix(s.outputs[i], s.outputs[i + 1], u);
                break;
            case AnimationSampler::CUBICSPLINE:
                value = cubicSpline(s, i, u, delta);
                break;
            }
        }

        switch (channel.path) {
        case AnimationChannel::TRANSLATION: pose.translation = glm::vec3(value); break;
        case AnimationChannel::SCALE:       pose.scale = glm::vec3(value); break;
        case AnimationChannel::ROTATION:    pose.rotation = glm::normalize(toQuat(value)); break;
        }
    }
}

namespace animation {

    void sampleClip(const AnimationClip& clip, float time, std::vector<NodePose>& pose) {
        for (const AnimationChannel& channel : clip.channels) {
            if (channel.node >= pose.size() || channel.samplerIndex >= clip.samplers.size()) continue;
            applyChannel(channel, clip.samplers[channel.samplerIndex], time, pose[channel.node]);
        }
    }

    void computeGlobalMatrices(const Skeleton& skeleton, const std::vector<NodePose>& pose, glm::mat4* globals) {
        for (uint32_t n : skeleton.evalOrder) {
            const NodePose& p = pose[n];
            const glm::mat4 local = glm::translate(glm::mat4(1.f), p.translation)
                * glm::mat4_cast(p.rotation)
                * glm::scale(glm::mat4(1.f), p.scale)
                * skeleton.nodes[n].matrix;

            const int32_t parent = skeleton.nodes[n].parent;
            globals[n] = parent >= 0 ? globals[parent] * local : local;
        }
    }

    void computeJointMatrices(const Skin& skin, const glm::mat4* globals, glm::mat4* out) {
        for (size_t i = 0; i < skin.joints.size(); ++i)
            out[i] = globals[skin.joints[i]] * skin.inverseBindMatrices[i];
    }
}

void Animator::init(const Skeleton& skeleton) {
    pose.resize(skeleton.nodes.size());
    for (size_t i = 0; i < skeleton.nodes.size(); ++i)
        pose[i] = skeleton.nodes[i].restPose;

    gpuMatrices.assign(skeleton.matrixCount, glm::mat4(1.f));
    computeMatrices(skeleton);
}

void Animator::play(uint32_t clipIndex, bool loopClip) {
    clip = clipIndex;
    loop = loopClip;
    playing = true;
    restart = true;
}

void Animator::stop() {
    playing = false;
}

bool Animator::update(float dt, const Skeleton& skeleton) {
    if (!playing) return false;
    if (clip >= skeleton.clips.size()) { playing = false; return false; }

    const AnimationClip& c = skeleton.clips[clip];
    const float duration = c.end - c.start;

    if (restart) { time = c.start; restart = false; }
    else time += dt;

    if (time > c.end) {
        if (loop && duration > 0.f) time = c.start + std::fmod(time - c.start, duration);
        else { time = c.end; playing = false; }
    }

    animation::sampleClip(c, time, pose);
    computeMatrices(skeleton);
    return true;
}

void Animator::computeMatrices(const Skeleton& skeleton) {
    if (gpuMatrices.empty()) return;

    animation::computeGlobalMatrices(skeleton, pose, gpuMatrices.data());
    for (const Skin& skin : skeleton.skins)
        animation::computeJointMatrices(skin, gpuMatrices.data(), gpuMatrices.data() + skin.jointOffset);
}