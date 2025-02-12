#include "Swap_chain.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <iostream>
#include <stdexcept>
#include <cassert>

#ifndef VK_SUBPASS_EXTERNAL
#define VK_SUBPASS_EXTERNAL (~0U)
#endif

Swap_chain::Swap_chain(Device& deviceRef, VkExtent2D windowExtent)
    : device{ deviceRef }, windowExtent{ windowExtent } {
    init();
}

Swap_chain::Swap_chain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<Swap_chain> previous) 
    : device{ deviceRef }, windowExtent{ windowExtent }, oldSwapChain{ previous } {
    init();


}

void Swap_chain::init()
{
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDepthResources(depthImages, depthImageMemorys, depthImageViews);
    createFramebuffers();
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

    for (int i = 0; i < depthImages.size(); i++) {
        vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
        vkDestroyImage(device.device(), depthImages[i], nullptr);
        vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
    }

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    }

    for (auto framebuffer : depthFramebuffers) {
        vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(device.device(), renderPass, nullptr);

    // cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.device(), inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < depthFinishedSemaphores.size(); ++i) {
        vkDestroySemaphore(device.device(), depthFinishedSemaphores[i], nullptr);
    }
}

VkResult Swap_chain::acquireNextImage(uint32_t* imageIndex) {
    vkWaitForFences(
        device.device(),
        1,
        &inFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(
        device.device(),
        swapChain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

VkResult Swap_chain::submitCommandBuffers(
    const VkCommandBuffer* buffers, uint32_t* imageIndex, bool waitDepthRender) {

    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    std::vector<VkSemaphore> tempWaitSemaphore{}; 

    if (waitDepthRender) {
        tempWaitSemaphore = { depthFinishedSemaphores };
    }

    tempWaitSemaphore.push_back(imageAvailableSemaphores[currentFrame]); 
    std::vector<VkPipelineStageFlags> waitStage(tempWaitSemaphore.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT); 


    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(tempWaitSemaphore.size());
    submitInfo.pWaitSemaphores = tempWaitSemaphore.data();// waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStage.data(); 

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
     
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}


void Swap_chain::submitDepthCommandBuffer(const std::vector<VkCommandBuffer> depthCommandBuffer)
{
    depthFinishedSemaphores.resize(depthCommandBuffer.size());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < depthCommandBuffer.size(); i++) { 
        if (depthFinishedSemaphores[i] == VK_NULL_HANDLE) { 
            if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &depthFinishedSemaphores[i]) != VK_SUCCESS) { 
                throw std::runtime_error("failed to create depth semaphore!");
            }
        }
    } 

    for (size_t i = 0; i < depthCommandBuffer.size(); i++) { 
        if (depthCommandBuffer[i] == VK_NULL_HANDLE) { 
            throw std::runtime_error("Error: Uninitialized command buffer in depthCommandBuffer!");
        }
    }

    // Submit Depth Pass 
    VkSubmitInfo depthSubmitInfo{};
    depthSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    depthSubmitInfo.commandBufferCount = static_cast<uint32_t>(depthCommandBuffer.size());
    depthSubmitInfo.pCommandBuffers = depthCommandBuffer.data();

    // Signal semaphore after depth pass
    depthSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(depthFinishedSemaphores.size());
    depthSubmitInfo.pSignalSemaphores = depthFinishedSemaphores.data();

    if (vkQueueSubmit(device.graphicsQueue(), 1, &depthSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit depth command buffer!");
    }
}
//
//VkResult Swap_chain::submitDepthCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
//{
//
//    // Wait for the fence of the current frame if it's in use
//    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
//        vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
//    }
//
//    imagesInFlight[*imageIndex] = inFlightFences[currentFrame]; 
//
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.commandBufferCount = 1; 
//    submitInfo.pCommandBuffers = buffers; 
//
//    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] }; 
//    submitInfo.signalSemaphoreCount = 1; 
//    submitInfo.pSignalSemaphores = signalSemaphores; 
//
//    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
//    if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
//        VK_SUCCESS) {
//        throw std::runtime_error("failed to submit draw command buffer!");
//    }
//
//    VkPresentInfoKHR presentInfo = {};
//    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//
//    presentInfo.waitSemaphoreCount = 1;
//    presentInfo.pWaitSemaphores = signalSemaphores;
//
//    VkSwapchainKHR swapChains[] = { swapChain };
//    presentInfo.swapchainCount = 1;
//    presentInfo.pSwapchains = swapChains;
//
//    presentInfo.pImageIndices = imageIndex;
//
//    auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);
//
//    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
//
//    return result;
//}

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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

void Swap_chain::createRenderPass() {

    std::cout << "render path creation \n";
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

   

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

}

void Swap_chain::createFramebuffers() {
    swapChainFramebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++) {
        std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageViews[i] };

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
            &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Swap_chain::createDepthResources(std::vector<VkImage>& image, std::vector<VkDeviceMemory>& imageMemory, std::vector<VkImageView>& imageView) {
    VkFormat depthFormat = findDepthFormat();
    swapChainDepthFormat = depthFormat;
    VkExtent2D swapChainExtent = getSwapChainExtent();

    image.resize(imageCount());
    imageMemory.resize(imageCount());
    imageView.resize(imageCount());

    for (int i = 0; i < image.size(); i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; 
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;
        imageInfo.pNext = NULL;

        device.createImageWithInfo( 
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image[i],
            imageMemory[i]);
       

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

    }
}

void Swap_chain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT); 
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT); 
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE); 

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
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
        if (availablepresentmode == VK_PRESENT_MODE_IMMEDIATE_KHR) { // vk_present_mode_mailbox_khr    vk_present_mode_immediate_khr
            std::cout << "present mode: mailbox" << std::endl;
            return availablepresentmode;
        }
    }

    // for (const auto &availablePresentMode : availablePresentModes) {
    //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //     std::cout << "Present mode: Immediate" << std::endl;
    //     return availablePresentMode;
    //   }
    // }

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

VkResult Swap_chain::submitDepthAndMainCommandBuffers(
    const std::vector<VkCommandBuffer> depthCommandBuffer,
    const VkCommandBuffer* mainCommandBuffer,
    uint32_t* imageIndex)
{
    // Wait for the fence of the current frame if it's in use
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    // Create the semaphore for depth pass signaling
    std::vector<VkSemaphore> depthFinishedSemaphore(depthCommandBuffer.size());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < depthCommandBuffer.size(); i++) {
        if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &depthFinishedSemaphore[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create depth semaphore!");
        }
    }
    
    // Submit Depth Pass
    VkSubmitInfo depthSubmitInfo{};
    depthSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    depthSubmitInfo.commandBufferCount = depthCommandBuffer.size();
    depthSubmitInfo.pCommandBuffers = depthCommandBuffer.data();

    // Signal semaphore after depth pass
    depthSubmitInfo.signalSemaphoreCount = depthFinishedSemaphore.size();
    depthSubmitInfo.pSignalSemaphores = depthFinishedSemaphore.data();

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    //VkPipelineStageFlags depthWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    std::vector<VkPipelineStageFlags> depthWaitStages(depthFinishedSemaphore.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    depthSubmitInfo.waitSemaphoreCount = 1;
    depthSubmitInfo.pWaitSemaphores = waitSemaphores;
    depthSubmitInfo.pWaitDstStageMask = depthWaitStages.data(); 


    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
    if (vkQueueSubmit(device.graphicsQueue(), 1, &depthSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit depth command buffer!");
    }

    // Submit Main Pass
    //VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
    std::vector<VkPipelineStageFlags> waitStages(depthFinishedSemaphore.size(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    VkSubmitInfo mainSubmitInfo{};
    mainSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    mainSubmitInfo.commandBufferCount = 1;
    mainSubmitInfo.pCommandBuffers = mainCommandBuffer; 

    // Wait for the depth pass to finish
    mainSubmitInfo.waitSemaphoreCount = depthFinishedSemaphore.size(); 
    mainSubmitInfo.pWaitSemaphores = depthFinishedSemaphore.data(); 
    mainSubmitInfo.pWaitDstStageMask = waitStages.data();

    // Signal the render finished semaphore for presentation
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    mainSubmitInfo.signalSemaphoreCount = 1;
    mainSubmitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device.graphicsQueue(), 1, &mainSubmitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit main command buffer!");
    }

    // Present the swapchain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

    vkQueueWaitIdle(device.presentQueue()); 

    // Clean up the depth semaphore
    for (VkSemaphore semaphore : depthFinishedSemaphore) {
        vkDestroySemaphore(device.device(), semaphore, nullptr);
    }
    
    // Increment the current frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}
