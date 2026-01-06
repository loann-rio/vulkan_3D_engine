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

    uint32_t frameIndex() const;


private:

    void recreateSwapchain();

    void createFrameContexts();

    void createCommandBuffers();
    void freeCommandBuffers();

    bool aquireFrame(FrameContext& frame);
    void presentFrame(FrameContext& frame);

    void beginFrame(FrameContext& frame);
    void endFrame(FrameContext& frame);

    Device& device;
    Window& window;

    std::unique_ptr<Swapchain> swapchain;

    std::unique_ptr<FrameRenderer> frameRenderer;

    std::vector<FrameContext> frames;
    std::vector<VkCommandBuffer> presentCommandBuffers;

    uint32_t currentFrameIndex = 0;

    bool isFrameStarted = false;
};
