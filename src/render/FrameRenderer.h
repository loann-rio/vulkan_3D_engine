#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

class Device;
class Swap_chain;

class FrameRenderer
{
public:

    FrameRenderer(
        Device& device,
        Swap_chain* swapchain
    );

    FrameRenderer(const FrameRenderer&) = delete;
    FrameRenderer& operator=(const FrameRenderer&) = delete;

    VkCommandBuffer beginFrame();

    VkResult endFrame();

   
    void recreate(Swap_chain* swapchain);

    VkCommandBuffer getCurrentCommandBuffer() const;

    uint32_t* getCurrentImageIndex();
  
    uint32_t getCurrentFrameIndex() const;

    bool isFrameInProgress() const;

private:

    void createCommandBuffers();
    void freeCommandBuffers();

private:

    Device& device;
    Swap_chain* swapchain;

    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentFrameIndex = 0;
    uint32_t currentImageIndex = 0;

    bool frameStarted = false;
};