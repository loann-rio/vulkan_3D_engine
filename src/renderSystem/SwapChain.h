#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <memory>

class Device;
class AssetManager;

class Swapchain {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    Swapchain(
        Device& device, 
        VkExtent2D windowExtent
    );
    
    Swapchain(
        Device& device, 
        VkExtent2D windowExtent, 
        std::shared_ptr<Swapchain> oldSwapchain
    );

    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // queries //

    uint32_t imageCount() const;
    VkExtent2D getExtent() const;
    VkFormat format() const;
    VkFormat depthFormat() const;

    // image //

    VkImage getImage(uint32_t imageIndex) const;
    VkImageView getImageView(uint32_t imageIndex) const;
    VkImageView getDepthImageView(uint32_t imageIndex) const;

    // Framebuffers //

    VkFramebuffer getFramebuffer(uint32_t imageIndex) const;
    VkRenderPass getDefaultRenderPass() const;

    // Synchronization / presentation //

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult present(uint32_t imageIndex);

    bool compareSwapFormat(const Swapchain& swapChain) const;

private:
    void createSwapchain(VkExtent2D extent);
    void createImageViews();
    void createDepthResources();
    //void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // helpers //
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
    Device& device;

    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
    std::shared_ptr<Swapchain> oldSwapchain;

    VkExtent2D extent{};
    VkFormat swapchainFormat{};
    VkFormat depthImageFormat{};

    // Swapchain images //

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    // Depth //

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthMemory;
    std::vector<VkImageView> depthImageViews;

    // Presentation sync //

    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame{ 0 };
};
