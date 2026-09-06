#include "ComputePass.h"
#include "../../base/descriptors.h"
#include <stdexcept>
#include <iostream>
#include <ctime>

ComputePass::ComputePass(Device& device, AssetManager& assets)
    : device{ device }, assets{ assets }
{
    auto setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        .build();

    ComputeSystemConfig config{
        "shaders/particule_update.comp.spv",
        { setLayout->getDescriptorSetLayout() },
        sizeof(float) + sizeof(int)
    };

    idk = std::make_shared<ComputeSystem>(device, assets, config);
}

ComputePass::~ComputePass()
{
}

void ComputePass::recordPass(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer, VkDescriptorSet instancesSet)
{
    struct pushConstant { float time; int count; };
    elapsedTime += frameInfo.frameTime;
    pushConstant push{ elapsedTime, 22500 };

    idk->dispatch(commandBuffer, frameInfo.frameIndex, { instancesSet }, 256, 1, 1, &push);
}