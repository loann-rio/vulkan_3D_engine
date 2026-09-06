#pragma once

#include "../../base/Device.h"
#include "../../assetManager/AssetManager.h"
#include "../../base/Frame_info.h"
#include "../ComputeSystem.h"

#include <vector>

class ComputePass {
public:
    ComputePass(
        Device& device, 
        AssetManager& assets
    );

    ~ComputePass();

    ComputePass(const ComputePass&) = delete;
    ComputePass& operator=(const ComputePass&) = delete;

    void recordPass(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer, VkDescriptorSet instancesSet);

private:
   
    Device& device;
    AssetManager& assets;

    std::shared_ptr<ComputeSystem> idk;
    std::vector<VkDescriptorSet> computeDescriptorSets;
    float elapsedTime = 0.0f;

};