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

    // Synchronization / presentation //

    VkResult acquireNextImage(uint32_t* imageIndex, uint32_t* outFrameSlot);
    VkResult present(uint32_t imageIndex);

    // Accessors for synchronization
    VkSemaphore getImageAvailableSemaphore(uint32_t frame) const;
    VkSemaphore getRenderFinishedSemaphore(uint32_t frame) const;
    VkFence getInFlightFence(uint32_t frame) const;

    bool compareSwapFormat(const Swapchain& swapChain) const;

private:
	void init(VkExtent2D extent);

    void createSwapchain(VkExtent2D extent);
    void createImageViews();
    void createSyncObjects();

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

    // Presentation sync //

    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame{ 0 };
};
