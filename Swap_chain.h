#pragma once

#include "Device.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>

class Swap_chain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Swap_chain(Device& deviceRef, VkExtent2D windowExtent);
    Swap_chain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swap_chain> previous);
    ~Swap_chain();

    Swap_chain(const Swap_chain&) = delete;
    Swap_chain& operator=(const Swap_chain&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }

    VkFramebuffer getDepthFramebuffers(int index) { return depthFramebuffers[index]; }
    VkRenderPass getRenderDepthPass() { return renderDepthPass; }
    VkImageView getDepthImageView(int index) { return depthImageViews[index]; }

    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }

    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
    VkResult submitDepthCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
    VkResult submitDepthAndMainCommandBuffers(
        const VkCommandBuffer* depthCommandBuffer,
        const VkCommandBuffer* mainCommandBuffer,
        uint32_t* imageIndex);

    bool compareSwapFormat(const Swap_chain& swapChain) const {
        return swapChain.swapChainDepthFormat == swapChainDepthFormat && 
            swapChain.swapChainImageFormat == swapChainImageFormat;
    }

private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources(std::vector<VkImage>& image, std::vector<VkDeviceMemory>& imageMemory, std::vector<VkImageView>& imageView);
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // depth pass
    void createRenderDepthPass();
    void createRenderDepthFramebuffers();


    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkFramebuffer> depthFramebuffers;
    VkRenderPass renderPass;
    VkRenderPass renderDepthPass;

    std::vector<VkImage> shadowDepthImages;
    std::vector<VkDeviceMemory> shadowDepthImageMemorys;
    std::vector<VkImageView> shadowDepthImageViews;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    Device& device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<Swap_chain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};