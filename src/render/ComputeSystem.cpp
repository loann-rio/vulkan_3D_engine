#include "ComputeSystem.h"
#include <stdexcept>

ComputeSystem::ComputeSystem(Device& device, AssetManager& assets, ComputeSystemConfig config)
    : device{ device }, assets{ assets }, pushSize{ config.pushConstantSize } {

    createPipelineLayout(config.globalLayouts);
    createComputePipeline(config.computeShader);

    hasPushConstants = (pushSize > 0);
}

ComputeSystem::~ComputeSystem() {
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }
}

void ComputeSystem::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts.data();

    VkPushConstantRange pushRange{};
    if (pushSize > 0) {
        pushRange.offset = 0;
        pushRange.size = static_cast<uint32_t>(pushSize);
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }
    else {
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;
    }

    if (vkCreatePipelineLayout(device.device(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout");
    }
}

void ComputeSystem::createComputePipeline(const std::string& compFile) {
    pipeline = std::make_unique<ComputePipeline>(device, compFile, pipelineLayout);
}

void ComputeSystem::dispatch(VkCommandBuffer commandBuffer, uint16_t frameIndex, std::vector<VkDescriptorSet> globalDescriptorSets,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, const void* pushConstants) {

    pipeline->bind(commandBuffer);

    // bind global + compute descriptor sets (order must match pipelineLayout set indices)
    for (uint32_t setIndex = 0; setIndex < globalDescriptorSets.size(); ++setIndex) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout,
            setIndex,
            1,
            &globalDescriptorSets[setIndex],
            0, nullptr);
    }

    if (hasPushConstants && pushConstants) {
        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            static_cast<uint32_t>(pushSize),
            pushConstants
        );
    }

    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);

/*    VkBuffer buffer = getBuffer();

    VkBufferMemoryBarrier bufferBarrier{};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = buffer;
    bufferBarrier.offset = 0;
    bufferBarrier.size = getBufferSize();

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,       // src stage
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,         // dst stage (vertex input)
        0,
        0, nullptr,
        1, &bufferBarrier,
        0, nullptr
    );*/
}