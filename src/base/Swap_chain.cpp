#include "Swap_chain.h"

// std
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "../render/PassTarget.h"

#ifndef VK_SUBPASS_EXTERNAL
#define VK_SUBPASS_EXTERNAL (~0U)
#endif

Swap_chain::Swap_chain(Device& deviceRef, AssetManager& assets, VkExtent2D windowExtent)
    : device{ deviceRef }, windowExtent{windowExtent} {
    init(assets);
}

Swap_chain::Swap_chain(Device& deviceRef, AssetManager& assets, VkExtent2D windowExtent, std::shared_ptr<Swap_chain> previous)
    : device{ deviceRef }, windowExtent{ windowExtent }, oldSwapChain{ previous } {
    init(assets);
}

void Swap_chain::init(AssetManager& assets)
{
    createSwapChain();
    createImageViews();
    swapChainDepthFormat = findDepthFormat();
    createSyncObjects();
}

Swap_chain::~Swap_chain() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device.device(), imageView, nullptr);
    }

    swapChainImageViews.clear(); 

    if (swapChain != nullptr) {
        vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
        swapChain = nullptr;
    }   

    // cleanup synchronization objects
    for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < inFlightFences.size(); i++) {
        if (inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(device.device(), inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < renderFinishedSemaphores.size(); ++i) {
        if (renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
    }
}

void Swap_chain::createFramebuffers(AssetManager& assets, PassTarget* textureTarget, VkRenderPass renderPass)
{
    textureTarget->framebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++) {
        //std::array<VkImageView, 2> attachments = { swapChainImageViews[i], assets.textures().get(textureTarget->getDepth(i))->view()};

        std::array<VkImageView, 1> attachments = { swapChainImageViews[i] };// , assets.textures().get(textureTarget->getDepth(i))->view() };

        VkExtent2D swapChainExtent = getSwapChainExtent();
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device.device(),
            &framebufferInfo,
            nullptr,
            &textureTarget->framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

VkResult Swap_chain::acquireNextImage(uint32_t* imageIndex) {
    vkWaitForFences(
        device.device(),
        1,
        &inFlightFences[(currentFrame + 1)%MAX_FRAMES_IN_FLIGHT],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(
        device.device(),
        swapChain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageIndex);                             // modified by vulkan

    return result;
}

VkResult Swap_chain::submitCommandBuffers(
    const VkCommandBuffer* buffers, uint32_t* imageIndex) {

    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    std::vector<VkSemaphore> tempWaitSemaphore{}; 

    tempWaitSemaphore = { depthFinishedSemaphores };
        
    tempWaitSemaphore.push_back(imageAvailableSemaphores[currentFrame]); 
    std::vector<VkPipelineStageFlags> waitStage(tempWaitSemaphore.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT); 

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(tempWaitSemaphore.size());
    submitInfo.pWaitSemaphores = tempWaitSemaphore.data();
    submitInfo.pWaitDstStageMask = waitStage.data(); 

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[*imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);

	auto result = device.submitAndPresent(submitInfo, inFlightFences[currentFrame], &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void Swap_chain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device.surface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

    if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // we only specified a minimum number of images in the swap chain, so the implementation is
    // allowed to create a swap chain with more. That's why we'll first query the final number of
    // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
    // retrieve the handles.
    vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Swap_chain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
}

void Swap_chain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT); 
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 

    renderFinishedSemaphores.resize(imageCount());
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE); 

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // create frame-based semaphores/fences
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    // create one render-finished semaphore per swapchain image
    for (size_t i = 0; i < renderFinishedSemaphores.size(); ++i) {
        if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create renderFinished semaphore for image!");
        }
    }
}

VkSurfaceFormatKHR Swap_chain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swap_chain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {

    for (const auto& availablepresentmode : availablePresentModes) {
        if (availablepresentmode == VK_PRESENT_MODE_MAILBOX_KHR) {
            std::cout << "present mode: mailbox" << std::endl;
            return availablepresentmode;
        }
    }

    /*for (const auto &availablePresentMode : availablePresentModes) {
       if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
         std::cout << "Present mode: Immediate" << std::endl;
         return availablePresentMode;
       }
    }*/

    std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swap_chain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = windowExtent; 
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkFormat Swap_chain::findDepthFormat() {
    return device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
