#pragma once

#include "FullScreenRenderSystem.h"

#include "../../base/descriptors.h"
#include "../../base/Device.h"

#include <exception>
#include <vulkan/vulkan_core.h>

FullScreenRenderSystem::FullScreenRenderSystem(
    Device& device,
    AssetManager& assets,
    const IVertexLayout* vertexLayout,
    const RenderSystemCreateInfo& createInfo
)
    : BaseRenderSystem(device, assets, vertexLayout, createInfo)
{
    createDescriptorSetLayouts(descriptorLayouts);
    createPipelineLayout(createInfo.globalSetLayout);
    createPipeline(createInfo);
}

bool FullScreenRenderSystem::accepts(const GameObjectModel& /*object*/) const
{ return false; }

BaseRenderSystem::PushConstantInfo FullScreenRenderSystem::pushConstants() const
{
    return { 0u, 0u };
}

void FullScreenRenderSystem::createDescriptorSetLayouts(
    std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
) {}

void FullScreenRenderSystem::renderModel(
    VkCommandBuffer /*cmd*/,
    FrameContext& /*frameContext*/,
    const RenderItem& /*item*/
) const {}

void FullScreenRenderSystem::renderFullScreen(VkCommandBuffer cmd, FrameContext& frameContext) const
{
    bindPipeline(cmd);
    
    //bindMaterial(cmd);

    vkCmdDraw(cmd, 3, 1, 0, 0);

}

void FullScreenRenderSystem::configurePipeline(PipelineConfigInfo& pipelineConfig) const
{
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
}
