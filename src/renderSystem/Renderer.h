#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "RenderPass/FrameRenderer.h"


class Device;
class Swapchain;

class GlobalRenderer {
public:
    GlobalRenderer(Device& device, Window& window, AssetManager& assetManager, ObjectManager& objectManager);
    ~GlobalRenderer();

    GlobalRenderer(const GlobalRenderer&) = delete;
    GlobalRenderer& operator=(const GlobalRenderer&) = delete;

    void renderFrame();
	uint32_t getCurrentFrameIndex() const { return currentFrameIndex; }
	uint32_t getNextFrameIndex() const { return (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT; }

private:
    
    void createSemaphore();
    void createFrameRenderer();

    void recreateSwapchain();

    bool aquireFrame();
    void presentFrame();

private:
    Device& device;
    Window& window;
    AssetManager& assetManager;
    ObjectManager& objectManager;

    FrameContext frameContext{};

    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<FrameRenderer> frameRenderer;

    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    uint64_t timelineValue = 0;

    uint32_t currentFrameIndex = 0; 

    uint64_t frameCounter = 0;      
    uint64_t lastCompletedFrame = 0; // cached timeline value

};
