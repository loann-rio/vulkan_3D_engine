#include "ShadowMeshRenderSystem.h"

#include "../../base/Device.h"

#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>


ShadowMeshRenderSystem::ShadowMeshRenderSystem(
	Device& device,
	AssetManager& assets,
	const IVertexLayout& vertexLayout,
	const RenderSystemCreateInfo& createInfo
)
	: BaseRenderSystem(device, assets, vertexLayout, createInfo)
{
	createDescriptorSetLayouts(descriptorLayouts);
	createPipelineLayout(createInfo.globalSetLayout);
	createPipeline(createInfo);
}


bool ShadowMeshRenderSystem::accepts(const GameObjectModel& object) const
{
	if (!object.modelAsset)
		return false;

	return true;
}

/// push constants

ShadowMeshRenderSystem::PushConstantInfo ShadowMeshRenderSystem::pushConstants() const
{
	return {
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(PushConstantData)
	};
}

/// rendering

void ShadowMeshRenderSystem::renderModel(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const RenderItem& item
) const
{
	assert(item.model);

	const uint32_t lodIndex = 0; // TODO: LOD selection
	const ModelLOD& lod = item.model->lods[lodIndex];

	drawLOD(
		cmd,
		frameContext,
		lod,
		item.modelMatrix
	);
}

void ShadowMeshRenderSystem::drawLOD(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const ModelLOD& lod,
	const glm::mat4& modelMat
) const
{

	if (lod.vertexBuffer && lod.vertexBuffer->getBuffer() != VK_NULL_HANDLE) {
		VkBuffer vb = lod.vertexBuffer->getBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
	}
	else {
		return;
	}

	if (lod.indexBuffer && lod.indexBuffer->getBuffer() != VK_NULL_HANDLE) {
		vkCmdBindIndexBuffer(cmd, lod.indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

	for (const Node* node : lod.nodes) {
		drawNode(cmd, frameContext, lod, node, modelMat);
	}
}

void ShadowMeshRenderSystem::drawNode(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const ModelLOD& lod,
	const Node* node,
	const glm::mat4& modelMat
) const
{
	for (const Node* child : node->children) 
		drawNode(cmd, frameContext, lod, child, modelMat);

	for (const Primitive& primitive : node->primitives) 
		drawPrimitive(cmd, primitive, modelMat);
}


void ShadowMeshRenderSystem::drawPrimitive(
	VkCommandBuffer cmd,
	const Primitive& primitive,
	const glm::mat4& modelMat
) const
{
	PushConstantData pc{};
	pc.modelMatrix = modelMat;

	vkCmdPushConstants(
		cmd,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PushConstantData),
		&pc
	);

	vkCmdDrawIndexed(
		cmd,
		primitive.indexCount,
		1,
		primitive.firstIndex,
		0,
		0
	);
}