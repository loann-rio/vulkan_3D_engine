 #pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <memory>

#include "../assetManager/AssetManager.h"

class Device;

class Swapchain {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    Swapchain(
        Device& device, 
        AssetManager& assets,
        VkExtent2D windowExtent
    );
    
    Swapchain(
        Device& device, 
        AssetManager& assets,
        VkExtent2D windowExtent, 
        std::shared_ptr<Swapchain> oldSwapchain
    );

    ~Swapchain();
    void destroySwapchainOnly();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // queries //

    uint32_t imageCount() const;
    VkExtent2D getExtent() const;
    VkFormat format() const;
    VkFormat depthFormat() const;

    // image //

    VkImage getImage(uint32_t imageIndex) const;
	const std::vector<VkImage>& getImages() const { return images; }
    VkImageView getImageView(uint32_t imageIndex) const;

	// final pass frameBuffer //
    VkFramebuffer getFramebuffer(uint32_t imageIndex) const;
	const std::vector<VkFramebuffer>& getFramebuffer() const { return swapChainFramebuffers; }

	//////// render loop functions ////////

    VkResult acquireNextImage(uint32_t* imageIndex);

    // submit
    void waitForImageInFlight(uint32_t imageIndex);
	void ResetFence();    
	VkFence getInFlightFence(uint32_t frame) const;  // or submit commandBuffers?

    // present
    VkResult present(uint32_t imageIndex);

   	VkSemaphore getImageAvailableSemaphore(uint32_t frame) const;
	VkSemaphore getRenderFinishedSemaphore(uint32_t frame) const;

    bool compareSwapFormat(const Swapchain& swapChain) const;

    void createFramebuffers(VkRenderPass renderPass);
private:
	void init(VkExtent2D extent);

    void createSwapchain(VkExtent2D extent);

    void createImageViews();
    void createDepthResources();
    void createSyncObjects();

	

private:
    Device& device;
    AssetManager& assets;

    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
    std::shared_ptr<Swapchain> oldSwapchain;

    VkExtent2D extent{};
    VkFormat swapchainFormat{};
    VkFormat depthImageFormat{};

    // Swapchain images //

    // color
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

	// depth
    std::vector<TextureManager::TextureID> depthTextures;  

    // Presentation sync //
    uint32_t currentFrame{ 0 };

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
};
