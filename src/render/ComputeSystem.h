#pragma once

#include "../base/device.h"
#include "../base/descriptors.h"
#include "../assetManager/AssetManager.h"
#include "../base/ComputePipeline.h"

#include <memory>
#include <vector>

struct ComputeSystemConfig {
    std::string computeShader;
    std::vector<VkDescriptorSetLayout> globalLayouts{};
    size_t pushConstantSize = 0;
};

class ComputeSystem {
public:
    ComputeSystem(
        Device& device, 
        AssetManager& assets, 
        ComputeSystemConfig config
    );

    ~ComputeSystem();

    ComputeSystem(const ComputeSystem&) = delete;
    ComputeSystem& operator=(const ComputeSystem&) = delete;

    // Bind global descriptor sets then dispatch
    void dispatch(
        VkCommandBuffer commandBuffer, 
        uint16_t frameIndex,
        std::vector<VkDescriptorSet> globalDescriptorSets,
        uint32_t groupCountX, 
        uint32_t groupCountY = 1, 
        uint32_t groupCountZ = 1,
        const void* pushConstants = nullptr
    );

private:
    void createPipelineLayout(
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts
    );

    void createComputePipeline(
        const std::string& compFile
    );

    Device& device;
    AssetManager& assets;

    std::unique_ptr<ComputePipeline> pipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    bool hasPushConstants = false;
    //  VkShaderStageFlags pushStage;
    size_t pushSize = 0;
};