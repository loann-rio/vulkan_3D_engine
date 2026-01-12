#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

#include "../objects/GameObject.h"
#include "SwapChain.h"

struct FrustumPlane;

struct FrameContext {
    uint32_t frameIndex{};
    uint32_t imageIndex{};

    VkCommandBuffer mainCommandBuffer;

    VkSemaphore imageAvailable;
    VkFence inFlightFence;

    VkSemaphore timeline;
    uint64_t timelineValue;

    VkFramebuffer swapchainFramebuffer;

    Swapchain* swapchain;

    std::vector<GameObjectModel*> listGameObjects;

    // cameras
    const std::array<FrustumPlane, 6>& planes;
};
