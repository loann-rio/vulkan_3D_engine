#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

struct FrustumPlane;

struct FrameContext {
    VkCommandBuffer commandBuffer{};

    uint32_t frameIndex{};
    uint32_t imageIndex{};

    std::vector<VkDescriptorSet> globalDescriptorSets;

    glm::mat4 view{};
    glm::mat4 proj{};

    std::array<FrustumPlane, 6> frustum{};
};
