#include "FullScreenRenderSystem.h"

FullScreenRenderSystem::FullScreenRenderSystem(Device& device, const RenderSystemCreateInfo& createInfo)
{
}

BaseRenderSystem::PushConstantInfo FullScreenRenderSystem::pushConstants() const
{
	return {
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(PushConstantData)
	};
}

void FullScreenRenderSystem::createDescriptorSetLayouts(std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts)
{
}

void FullScreenRenderSystem::bindMaterial(VkCommandBuffer cmd, const ModelLOD& lod, uint32_t materialIndex, uint32_t frameIndex) const
{
}
