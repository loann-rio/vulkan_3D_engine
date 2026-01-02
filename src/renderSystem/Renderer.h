#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "RenderPass/FrameRenderer.h"

class Device;
class Swapchain;

class Renderer {
public:
    Renderer(Device& device, Swapchain& swapchain);

    void renderFrame();

private:
    void beginCommandBuffer(VkCommandBuffer cmd);
    void endCommandBuffer(VkCommandBuffer cmd);

private:
    Device& device;
    Swapchain& swapchain;

    FrameRenderer frameRenderer;

    std::vector<VkCommandBuffer> commandBuffers;
};
