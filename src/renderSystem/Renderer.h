#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "RenderPass/FrameRenderer.h"


class Device;
class Swapchain;

class GlobalRenderer {
public:
    GlobalRenderer(Device& device, Window& window);
    ~GlobalRenderer();

    GlobalRenderer(const GlobalRenderer&) = delete;
    GlobalRenderer& operator=(const GlobalRenderer&) = delete;

    void renderFrame();

private:
    

    void createFrameContexts();
    void createSemaphore();
    void createFrameRenderer();

    void recreateSwapchain();

    bool aquireFrame();
    void presentFrame();

private:
    Device& device;
    Window& window;

    FrameContext frameContext;

    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<FrameRenderer> frameRenderer;

    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    uint64_t timelineValue = 0;

    uint32_t currentFrameIndex = 0; 
};
