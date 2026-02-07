#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>

#include "../objects/GameObject.h"
#include "SwapChain.h"

struct FrustumPlane;

struct RenderItem {
    const ModelAsset* model;
    glm::mat4 modelMatrix;
    glm::mat4 normalMatrix;
};

struct FrameContext {
    uint32_t frameIndex{};
    uint32_t imageIndex{};

    VkCommandBuffer mainCommandBuffer{};

    VkSemaphore imageAvailable{};

    uint64_t timelineValue{1};
	VkSemaphore timelineSemaphore{};

    VkFramebuffer* swapchainFramebuffer;

    Swapchain* swapchain{};

    std::vector<GameObjectModel*> listGameObjects;
    std::vector<RenderItem> renderItems;

    // cameras
    std::array<FrustumPlane, 6>* planes;

	VkDescriptorSet globalSet{};
};
