#pragma once

#include "../base/Device.h"
#include "../assetManager/AssetManager.h"
#include "../base/Frame_info.h"
#include "ComputeSystem.h"

#include <vector>

class ComputePass {
public:
    ComputePass(
        Device& device, 
        AssetManager& assets, 
        std::vector<VkDescriptorSetLayout> globalLayouts = {}
    );

    ~ComputePass();

    ComputePass(const ComputePass&) = delete;
    ComputePass& operator=(const ComputePass&) = delete;

    void recordPass(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer, VkDescriptorSet instancesSet);

private:
   
    Device& device;
    AssetManager& assets;

    std::shared_ptr<ComputeSystem> idk;

    VkDescriptorSetLayout localSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> computeDescriptorSets;

    std::vector<VkDescriptorSetLayout> globalLayouts; // passed in from renderer to keep set order
};