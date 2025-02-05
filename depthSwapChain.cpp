#include "depthSwapChain.h"

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

DepthSwapChain::DepthSwapChain(Device& deviceRef, VkExtent2D depthImageExtent)
	: device{ deviceRef }, depthExtent { depthImageExtent }
{
	init();
}

DepthSwapChain::~DepthSwapChain()
{
	if (depthRenderPass != nullptr) {
		vkDestroyRenderPass(device.device(), depthRenderPass, nullptr);
	}

	for (int i = 0; i < depthImages.size(); i++) {
		vkDestroyImageView( device.device(), depthImageViews[i],   nullptr);
		vkDestroyImage(     device.device(), depthImages[i],       nullptr);
		vkDestroySampler(   device.device(), depthSampler[i],      nullptr); 
		vkFreeMemory(       device.device(), depthImageMemorys[i], nullptr);
	}

	for (auto framebuffer : depthFramebuffers) {
		vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
	}
}

VkFormat DepthSwapChain::findDepthFormat()
{
    return device.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void DepthSwapChain::transitionDepthImageLayout(VkCommandBuffer& depthCommandBuffer, int depthFrameIndex, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    device.transitionImageLayout(depthCommandBuffer, depthImages[depthFrameIndex], swapChainDepthFormat, oldLayout, newLayout, 1);
}

void DepthSwapChain::init()
{
    createDepthRenderPass();
    createDepthResources();
    createDepthbuffers();
}

void DepthSwapChain::createDepthResources()
{
    swapChainDepthFormat = findDepthFormat(); 

    depthImages         .resize(DEPTH_RENDER_COUNT);
    depthImageMemorys   .resize(DEPTH_RENDER_COUNT);
    depthImageViews     .resize(DEPTH_RENDER_COUNT);
    depthSampler        .resize(DEPTH_RENDER_COUNT);

    for (int i = 0; i < DEPTH_RENDER_COUNT; i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = depthExtent.width;
        imageInfo.extent.height = depthExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = swapChainDepthFormat;
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
            depthImages[i],
            depthImageMemorys[i]);


        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainDepthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

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


        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &depthSampler[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

        device.transitionImageLayout(depthImages[i], swapChainDepthFormat,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }
}

void DepthSwapChain::createDepthRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store depth results
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = 0;  // The first subpass
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // Fragment shader reads the image
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;  // Depth attachment write
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;  // The image is being read in the fragment shader 
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // Depth attachment write
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &depthRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pre-render depth pass!");
    }
}

void DepthSwapChain::createDepthbuffers()
{
    depthFramebuffers.resize(DEPTH_RENDER_COUNT);
    for (size_t i = 0; i < DEPTH_RENDER_COUNT; i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = depthRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &depthImageViews[i]; // Depth only
        framebufferInfo.width = depthExtent.width;
        framebufferInfo.height = depthExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device.device(),
            &framebufferInfo,
            nullptr,
            &depthFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pre-render depth framebuffer!");
        }
    }
}
