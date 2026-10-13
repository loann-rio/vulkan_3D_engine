#pragma once
#include <vector>
#include <string>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct NodePose {
    glm::vec3 translation{ 0.f };
    glm::quat rotation{ 1.f, 0.f, 0.f, 0.f }; 
    glm::vec3 scale{ 1.f };
};

struct SkeletonNode {
    int32_t parent = -1;
    NodePose restPose;            
    glm::mat4 matrix{ 1.f };
    std::string name;
};

struct Skin {     
    std::string name;
    int32_t skeletonRoot = -1;
    std::vector<uint32_t> joints;
    std::vector<glm::mat4> inverseBindMatrices;
};

struct AnimationSampler {
    enum InterpolationType { LINEAR, STEP, CUBICSPLINE } interpolation = LINEAR;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputs;
};

struct AnimationChannel {
    enum PathType { TRANSLATION, ROTATION, SCALE } path;
    uint32_t node;
    uint32_t samplerIndex;
};

struct AnimationClip {
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = 0.f;
};

struct Skeleton {
    std::vector<SkeletonNode> nodes;
    std::vector<uint32_t> evalOrder;
    std::vector<Skin> skins;
    std::vector<AnimationClip> clips;

    bool empty() const { return nodes.empty(); }
};
