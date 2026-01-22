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

private:

    void createUboDescriptorPool();
    
    void createSemaphore();
    void createFrameRenderer();
	void createFrameBuffers();

    void recreateSwapchain();

    bool aquireNextImage();
    void presentFrame();

private:
    Device& device;
    Window& window;
    AssetManager& assetManager;
    ObjectManager& objectManager;

    std::unique_ptr<DescriptorPool> globalPool; 

    FrameContext frameContext{};

    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<FrameRenderer> frameRenderer;

    uint32_t currentImageIndex;
    int currentFrameIndex = 0; 

};
