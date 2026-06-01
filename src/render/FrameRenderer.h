#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

class Device;
class Swap_chain;

/*
    Contains all GPU synchronization and
    per-frame command buffer management.

    Responsibilities:
    - frame begin/end
    - frame synchronization
    - acquire/present
    - command buffer lifecycle
    - frame indexing

    Does NOT:
    - perform rendering
    - know about render passes
    - know about scenes
    - know about render systems
*/
class FrameRenderer
{
public:

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameData
    {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

        VkFence inFlightFence = VK_NULL_HANDLE;
    };

public:

    FrameRenderer(
        Device& device,
        Swap_chain* swapchain
    );

    ~FrameRenderer();

    FrameRenderer(const FrameRenderer&) = delete;
    FrameRenderer& operator=(const FrameRenderer&) = delete;

    /*
        Begin a new frame.

        Handles:
        - fence wait/reset
        - image acquisition
        - command buffer begin
    */
    VkCommandBuffer beginFrame();

    /*
        End current frame.

        Handles:
        - command buffer end
        - queue submit
        - present
    */
    VkResult endFrame();

    /*
        Called after swapchain recreation
    */
    void recreate(Swap_chain* swapchain);

    /*
        Current frame command buffer
    */
    VkCommandBuffer getCurrentCommandBuffer() const;

    /*
        Current swapchain image index
    */
    uint32_t* getCurrentImageIndex();

    /*
        Current CPU frame index
    */
    uint32_t getCurrentFrameIndex() const;

    /*
        Current frame synchronization data
    */
    const FrameData& getCurrentFrameData() const;

    /*
       is frame currently recording
    */
    bool isFrameInProgress() const;

private:

    void createCommandBuffers();
    void freeCommandBuffers();

    void createSyncObjects();
    void destroySyncObjects();

private:

    Device& device;
    Swap_chain* swapchain;

    std::vector<VkCommandBuffer> commandBuffers;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frames;

    uint32_t currentFrameIndex = 0;
    uint32_t currentImageIndex = 0;

    bool frameStarted = false;
};