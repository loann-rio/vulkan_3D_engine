#pragma once

#include "../base/Device.h"
#include "../base/descriptors.h"
#include "../base/Swap_chain.h"
#include "../assetManager/AssetManager.h"

#include <vulkan/vulkan_core.h>
#include <vector>


class PassTarget {

public:
    PassTarget(
        Device& device_,
        Swap_chain& swapchain_,
        AssetManager& assets_,
        VkExtent2D extent_,
        bool hasDepth,
        bool hasColor,
        size_t imageCount = 1,
        bool isFinal = false
    );

    ~PassTarget();

    void resizeTargets(
        VkExtent2D newExtent, 
        uint32_t imageCount, 
        VkFormat format, 
        VkFormat depthFormat
    );

    void createLocalFramebuffers(
        VkRenderPass renderPass
    );

    void createDescriptorSets(
        DescriptorPool& pool
    );

    VkDescriptorSet getColorDescriptorSet(uint32_t index) { return colorDescriptorSets[index]; }
    VkDescriptorSet getDepthDescriptorSet(uint32_t index) { return depthDescriptorSets[index]; }

    VkDescriptorImageInfo getColorImageInfo(uint16_t index) const { return colorImageInfo[index]; }
    VkDescriptorImageInfo getDepthImageInfo(uint16_t index) const { return depthImageInfo[index]; }


    TextureManager::TextureID getColor(uint16_t index) const { return color[index]; }
    TextureManager::TextureID getDepth(uint16_t index) const { return depth[index]; }

    VkFramebuffer getFrameBuffer(uint16_t index) const { return framebuffers[index]; }

    VkExtent2D getExtent() const { return extent; }


private:
    void cleanupLocalFramebuffers();
    void cleanupTargetTextures();

    void createColorTargetTexture(
        uint32_t imageCount, 
        VkFormat format, 
        std::vector<VkImage> swapImage = {}
    );

    void createDepthTargetTexture(
        uint32_t imageCount, 
        VkFormat format
    );

    void createImageInfo();

    std::vector<TextureManager::TextureID> color;
    std::vector<TextureManager::TextureID> depth;

    std::vector<VkDescriptorSet> colorDescriptorSets;
    std::vector<VkDescriptorSet> depthDescriptorSets;

    std::vector<VkDescriptorImageInfo> colorImageInfo;
    std::vector<VkDescriptorImageInfo> depthImageInfo;

    std::vector<VkFramebuffer> framebuffers;

    VkExtent2D extent;

    AssetManager& assets;
    Device& device;
};