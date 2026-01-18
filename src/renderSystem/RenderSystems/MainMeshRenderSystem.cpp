#include "MainMeshRenderSystem.h"

#include "../../base/descriptors.h"
#include "../../base/Device.h"

#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>


MainMeshRenderSystem::MainMeshRenderSystem(
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


bool MainMeshRenderSystem::accepts(const GameObjectModel& object) const
{
	if (!object.modelAsset)
		return false;

	const ModelAsset* model = assets.models().get(object.modelAsset);


	return (
		(static_cast<int>(model->type) == static_cast<int>(AssetModelType::STATIC_MESH)) && 
		(vertexLayout.isCompatibleWith(model->lods[0].vertexLayout))
		);
}



/// push constants

BaseRenderSystem::PushConstantInfo MainMeshRenderSystem::pushConstants() const
{
	return {
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(PushConstantData)
	};
}

/// descriptor set layouts

void MainMeshRenderSystem::createDescriptorSetLayouts(
	std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
)
{

	/// material layout
	auto materialLayout = DescriptorSetLayout::Builder(device)
		.addBinding(
			0,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT
		)
		.build();

	outLayouts.push_back(std::move(materialLayout));
}

/// rendering

void MainMeshRenderSystem::renderModel(
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
		item.modelMatrix,
		item.normalMatrix
	);
}

void MainMeshRenderSystem::drawLOD(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const ModelLOD& lod,
	const glm::mat4& modelMat,
	const glm::mat4& normalMat
) const
{
	for (const Node* node : lod.nodes) {
		drawNode(cmd, frameContext, lod, node, modelMat, normalMat);
	}
}

void MainMeshRenderSystem::drawNode(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const ModelLOD& lod,
	const Node* node,
	const glm::mat4& modelMat,
	const glm::mat4& normalMat
) const
{
	for (const Node* child : node->children) {
		drawNode(cmd, frameContext, lod, child, modelMat, normalMat);
	}

	for (const Primitive& primitive : node->primitives) {
		bindMaterial(
			cmd,
			lod,
			primitive.materialIndex,
			frameContext.frameIndex
		);

		drawPrimitive(cmd, primitive, modelMat, normalMat);
	}
}

void MainMeshRenderSystem::bindMaterial(
	VkCommandBuffer cmd,
	const ModelLOD& lod,
	uint32_t materialIndex,
	uint32_t frameIndex
) const
{
	VkDescriptorSet set =
		lod.materials[materialIndex].descriptorSet[frameIndex];

	uint32_t materialSetIndex = 1; // TODO: make configurable

	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		materialSetIndex,
		1,      /*    descriptor count    */
		&set,   /*    pDescriptorSets     */
		0,      /*  dynamic offset count  */
		nullptr /*    pDynamicOffsets     */
	);
}

void MainMeshRenderSystem::drawPrimitive(
	VkCommandBuffer cmd,
	const Primitive& primitive,
	const glm::mat4& modelMat,
	const glm::mat4& normalMat
) const
{
	PushConstantData pc{};
	pc.modelMatrix = modelMat;
	pc.normalMatrix = normalMat;

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