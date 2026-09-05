#include "ComputePass.h"
#include "../../base/descriptors.h"
#include <stdexcept>

ComputePass::ComputePass(Device& device, AssetManager& assets)
    : device{ device }, assets{ assets }
{

    auto setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)// DepthPass::MAX_DEPTH_RENDER_COUNT)
        .build();

    ComputeSystemConfig config{
        "shaders/particule_update.comp.spv",
        {setLayout->getDescriptorSetLayout()}, 
        sizeof(float) + sizeof(int)
    };

    idk = std::make_shared<ComputeSystem>(device, assets, config);
}

ComputePass::~ComputePass()
{
    if (localSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device.device(), localSetLayout, nullptr);
}

void ComputePass::recordPass(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer, VkDescriptorSet instancesSet)
{
    struct pushConstant { float time; int count; };
    pushConstant push{ frameInfo.frameTime, 22500 };

    idk->dispatch(commandBuffer, frameInfo.frameIndex, {instancesSet}, 64, 1, 1, &push);
}