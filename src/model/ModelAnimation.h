#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <limits>
#include <glm/glm.hpp>

class Node;

struct AnimationChannel
{
    enum PathType { TRANSLATION, ROTATION, SCALE };
    PathType path;
    Node* node;
    uint32_t samplerIndex;
};

struct AnimationSampler
{
    enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;
    std::vector<float> outputs;
    glm::vec4 cubicSplineInterpolation(size_t index, float time, uint32_t stride);
    void translate(size_t index, float time, Node* node);
    void scale(size_t index, float time, Node* node);
    void rotate(size_t index, float time, Node* node);
};

struct Animation
{
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
};

struct Skin {
    std::string name;
    Node* skeletonRoot;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node*> joints;
};
