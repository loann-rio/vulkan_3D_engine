#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Skeleton.h"

namespace animation {
    // write the values of the clip at `time` in the pose (one NodePose per skeleton node)
    void sampleClip(const AnimationClip& clip, float time, std::vector<NodePose>& pose);

    // local TRS -> model space matrices, evaluated in skeleton.evalOrder
    void computeGlobalMatrices(const Skeleton& skeleton, const std::vector<NodePose>& pose, glm::mat4* globals);

    // global joint matrix * inverse bind matrix
    void computeJointMatrices(const Skin& skin, const glm::mat4* globals, glm::mat4* out);
}

class Animator {
public:
    void init(const Skeleton& skeleton);

    void play(uint32_t clipIndex, bool loop = true);
    void stop();
    bool isPlaying() const { return playing; }

    // advance the clip and recompute the matrices, returns true when they changed
    bool update(float dt, const Skeleton& skeleton);

    const std::vector<glm::mat4>& matrices() const { return gpuMatrices; }

private:
    void computeMatrices(const Skeleton& skeleton);

    uint32_t clip = 0;
    float time = 0.f;
    bool playing = false;
    bool loop = true;
    bool restart = false;

    std::vector<NodePose> pose;
    std::vector<glm::mat4> gpuMatrices;
};
