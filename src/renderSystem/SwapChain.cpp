#include "SwapChain.h"

#include "../base/Device.h"

#include <stdexcept>
#include <cassert>
#include <limits>
#include <algorithm>

namespace {
    VkSurfaceFormatKHR chooseSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& formats) {
        for (const auto& f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        return formats[0];
    }

    VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& modes) {
        for (auto m : modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                return m;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseExtent(
        const VkSurfaceCapabilitiesKHR& caps, VkExtent2D extent) {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return caps.currentExtent;

        VkExtent2D actual = extent;
        actual.width = std::clamp(actual.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        return actual;
    }

    VkFormat findDepthFormat(Device& device) {
        return device.findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }


}

Swapchain::Swapchain(Device& device, VkExtent2D windowExtent)
    : device(device) {
	init(windowExtent);
}

Swapchain::Swapchain(Device& device,
    VkExtent2D windowExtent,
    std::shared_ptr<Swapchain> old)
    : device(device), oldSwapchain(old) {
	init(windowExtent);
}

void Swapchain::init(VkExtent2D extent)
{
    createSwapchain(extent);
    createImageViews();
    createSyncObjects();
}


Swapchain::~Swapchain()
{
    for (auto fence : inFlightFences)
        vkDestroyFence(device.device(), fence, nullptr);

    for (auto sem : imageAvailable)
        vkDestroySemaphore(device.device(), sem, nullptr);

    for (auto sem : renderFinished)
        vkDestroySemaphore(device.device(), sem, nullptr);

    for (auto view : imageViews)
        vkDestroyImageView(device.device(), view, nullptr);

    if (swapchain)
        vkDestroySwapchainKHR(device.device(), swapchain, nullptr);
}

uint32_t Swapchain::imageCount() const {
    return static_cast<uint32_t>(images.size());
}

VkExtent2D Swapchain::getExtent() const {
    return extent;
}

VkFormat Swapchain::format() const {
    return swapchainFormat;
}

VkFormat Swapchain::depthFormat() const {
    return depthImageFormat;
}

VkImage Swapchain::getImage(uint32_t index) const {
    return images[index];
}

VkImageView Swapchain::getImageView(uint32_t index) const {
    return imageViews[index];
}

VkResult Swapchain::acquireNextImage(uint32_t* imageIndex, uint32_t* outFrameSlot) {
    vkWaitForFences(
        device.device(),
        1,
        &inFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(
        device.device(),
        swapchain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailable[currentFrame],
        VK_NULL_HANDLE,
        imageIndex);

    if (outFrameSlot) *outFrameSlot = currentFrame;

    return result;
}

VkResult Swapchain::present(uint32_t imageIndex) {
    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished[currentFrame];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(device.presentQueue(), &present);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return result;
}

bool Swapchain::compareSwapFormat(const Swapchain& swapChain) const
{
    return swapchainFormat == swapChain.swapchainFormat &&
        depthImageFormat == swapChain.depthImageFormat;
}

void Swapchain::createSwapchain(VkExtent2D windowExtent)
{
    depthImageFormat = findDepthFormat(device);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device.getPhysicalDevice(), 
        device.surface(), 
        &capabilities
    );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device.getPhysicalDevice(), 
        device.surface(), 
        &formatCount,
        nullptr
    );

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device.getPhysicalDevice(), 
        device.surface(), 
        &formatCount, 
        formats.data()
    );

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.getPhysicalDevice(), 
        device.surface(), 
        &presentModeCount, 
        nullptr
    );

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.getPhysicalDevice(), 
        device.surface(), 
        &presentModeCount, 
        presentModes.data()
    );

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
    VkPresentModeKHR presentMode = choosePresentMode(presentModes);
    extent = chooseExtent(capabilities, extent);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create{};
    create.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create.surface = device.surface();
    create.minImageCount = imageCount;
    create.imageFormat = surfaceFormat.format;
    create.imageColorSpace = surfaceFormat.colorSpace;
    create.imageExtent = extent;
    create.imageArrayLayers = 1;
    create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create.preTransform = capabilities.currentTransform;
    create.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create.presentMode = presentMode;
    create.clipped = VK_TRUE;
    create.oldSwapchain = oldSwapchain ? oldSwapchain->swapchain : VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device.device(), &create, nullptr, &swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain");

    vkGetSwapchainImagesKHR(device.device(), swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device.device(), swapchain, &imageCount, images.data());

    swapchainFormat = surfaceFormat.format;
}

void Swapchain::createImageViews() {
    imageViews.resize(images.size());

    for (size_t i = 0; i < images.size(); ++i) {
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = images[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = swapchainFormat;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &view, nullptr, &imageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain image view");
    }
}

void Swapchain::createSyncObjects() {
    imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo sem{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fence{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device.device(), &sem, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.device(), &sem, nullptr, &renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(device.device(), &fence, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects");
    }
}

VkSemaphore Swapchain::getImageAvailableSemaphore(uint32_t frame) const {
    return imageAvailable.at(frame);
}
VkSemaphore Swapchain::getRenderFinishedSemaphore(uint32_t frame) const {
    return renderFinished.at(frame);
}
VkFence Swapchain::getInFlightFence(uint32_t frame) const {
    return inFlightFences.at(frame);
}