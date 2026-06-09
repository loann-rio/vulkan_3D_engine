#pragma once

#include "Device.h"

#include "../assetManager/AssetManager.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <vector>
#include <memory>

class PassTarget;

class Swap_chain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Swap_chain(Device& deviceRef, AssetManager& assets, VkExtent2D windowExtent);
    Swap_chain(Device& deviceRef, AssetManager& assets, VkExtent2D windowExtent, std::shared_ptr<Swap_chain> previous);
    ~Swap_chain();

    Swap_chain(const Swap_chain&) = delete;
    Swap_chain& operator=(const Swap_chain&) = delete;

    VkImageView getImageView(int index) { return swapChainImageViews[index]; }

    size_t imageCount() const { return swapChainImages.size(); }
    std::vector<VkImage> getSwapChainImages() const { return swapChainImages; }

    VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
    VkFormat getSwapChainDepthFormat() const { return swapChainDepthFormat; }

    VkExtent2D getSwapChainExtent() const { return swapChainExtent; }

    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    void createFramebuffers(AssetManager& assets, PassTarget* textureTarget, VkRenderPass renderPass);

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    bool compareSwapFormat(const Swap_chain& swapChain) const {
        return swapChain.swapChainDepthFormat == swapChainDepthFormat && 
            swapChain.swapChainImageFormat == swapChainImageFormat;
    }

private:
    void init(AssetManager& assets);
    void createSwapChain();
    void createImageViews();
    void createSyncObjects();

    VkFormat findDepthFormat();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkExtent2D swapChainExtent;

    Device& device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<Swap_chain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> depthFinishedSemaphores;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    size_t currentFrame = 0;
};