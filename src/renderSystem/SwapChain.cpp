#include "SwapChain.h"

#include "../base/Device.h"

#include <stdexcept>
#include <cassert>
#include <limits>
#include <array>
#include <algorithm>
#include <iostream>

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
        /*for (const auto& availablepresentmode : modes) {
        if (availablepresentmode == VK_PRESENT_MODE_MAILBOX_KHR) {
            std::cout << "present mode: mailbox" << std::endl;
            return availablepresentmode;
        }
        }*/

        /*for (const auto &availablePresentMode : modes) {
           if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
             std::cout << "Present mode: Immediate" << std::endl;
             return availablePresentMode;
           }
        }*/

        std::cout << "Present mode: V-Sync" << std::endl;
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

Swapchain::Swapchain(Device& device, AssetManager& assets, VkExtent2D windowExtent)
    : device(device), assets(assets) {
	init(windowExtent);
}

Swapchain::Swapchain(Device& device,
    AssetManager& assets,
    VkExtent2D windowExtent,
    std::shared_ptr<Swapchain> old)
    : device(device), assets(assets), oldSwapchain(old) {
	init(windowExtent);
}

void Swapchain::init(VkExtent2D extent)
{
    createSwapchain(extent);
    createImageViews();
    createDepthResources();
    createSyncObjects();
}
 

Swapchain::~Swapchain()
{
    for (auto fence : inFlightFences)
        vkDestroyFence(device.device(), fence, nullptr);

    for (auto fence : imagesInFlight)
        vkDestroyFence(device.device(), fence, nullptr);

    for (auto view : imageViews)
        vkDestroyImageView(device.device(), view, nullptr);

	for (auto fb : swapChainFramebuffers)
        vkDestroyFramebuffer(device.device(), fb, nullptr);
     
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

VkFramebuffer Swapchain::getFramebuffer(uint32_t imageIndex) const
{
	return swapChainFramebuffers[imageIndex];
}

VkResult Swapchain::acquireNextImage(uint32_t* imageIndex) {
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
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}


void Swapchain::ResetFence()
{
    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
}

VkResult Swapchain::present(uint32_t imageIndex) {

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;

    present.pImageIndices = &imageIndex;
    

    VkResult result = device.present(&present);

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
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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

void Swapchain::createDepthResources()
{
    depthTextures.resize(imageCount());

    for (int i = 0; i < depthTextures.size(); i++) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthImageFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;
        imageInfo.pNext = NULL;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 100.0f;

        TextureBuilder builder(device);
        depthTextures[i] = assets.textures().create(builder.fromTextureInfo(imageInfo, viewInfo, samplerInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }

}

void Swapchain::createFramebuffers(VkRenderPass renderPass)
{
    swapChainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i) {
        std::array<VkImageView, 2> attachments = {
            imageViews[i],
            assets.textures().get(depthTextures[i])->view()
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device.device(),
            &framebufferInfo,
            nullptr,
            &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Swapchain: failed to create framebuffer");
        }
    }
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

void Swapchain::waitForImageInFlight(uint32_t imageIndex)
{
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
}

VkFence Swapchain::getInFlightFence(uint32_t frame) const {
    return inFlightFences.at(frame);
}

VkSemaphore Swapchain::getImageAvailableSemaphore(uint32_t frame) const
{
	return imageAvailableSemaphores.at(frame);
}

VkSemaphore Swapchain::getRenderFinishedSemaphore(uint32_t frame) const
{
	return renderFinishedSemaphores.at(frame);
}
