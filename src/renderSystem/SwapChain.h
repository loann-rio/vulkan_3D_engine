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

    struct Frame {
        VkImage image{};
        VkImageView imageView{};
        VkFramebuffer framebuffer{};
        VkCommandBuffer commandBuffer{};
        VkSemaphore imageAvailable{};
        VkSemaphore renderFinished{};
        VkFence inFlight{};
    };

    Swapchain(
        Device& device,
        AssetManager& assets,
        VkExtent2D extent,
        std::shared_ptr<Swapchain> oldSwapchain = nullptr
    );

    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    VkExtent2D extent() const { return extent_; }
    VkFormat imageFormat() const { return imageFormat_; }
    VkRenderPass renderPass() const { return renderPass_; }

    uint32_t currentFrameIndex() const { return currentFrame_; }
    const Frame& currentFrame() const { return frames_[currentFrame_]; }

    bool acquireNextImage(uint32_t& imageIndex);
    void present(uint32_t imageIndex);

    void advanceFrame();

private:
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

private:
    Device& device;
    AssetManager& assets;

    VkSurfaceKHR surface_{};

    VkSwapchainKHR swapchain_{};
    VkFormat imageFormat_{};
    VkExtent2D extent_{};

    VkRenderPass renderPass_{};

    std::vector<VkImage> images_;
    std::vector<Frame> frames_;

    uint32_t currentFrame_{ 0 };
    uint32_t maxFrames_{ 0 };
};
