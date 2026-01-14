#include "MainMeshRenderSystem.h"

#include "../../base/descriptors.h"
#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vector>
#include "../../base/Device.h"
#include "BaseRenderSystem.h"
#include <vulkan/vulkan_core.h>




MainMeshRenderSystem::MainMeshRenderSystem(Device& device, AssetManager& assets, IVertexLayout* vertexLayout, RenderSystemConfig& config)
	: BaseRenderSystem(device, assets, vertexLayout, config) { }

VkShaderStageFlagBits MainMeshRenderSystem::pushStage() const
{
	return VK_SHADER_STAGE_VERTEX_BIT;
}

uint32_t MainMeshRenderSystem::pushSize() const
{
	return sizeof(PushConstantData);
}

std::vector<VkDescriptorSetLayout> MainMeshRenderSystem::createSetLayout(RenderSystemConfig& config)
{
	std::vector<VkDescriptorSetLayout> layouts;  // to hold the unique ptr of the descriptor set layouts

	//modelDescriptorSetIndex = static_cast<uint32_t>(config.globalSetLayout.size()); // index of the model descriptor set in the pipeline layout

	auto builder = DescriptorSetLayout::Builder(device);
	std::unique_ptr<DescriptorSetLayout> newLayout = builder
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
		.build();

	layouts.push_back(newLayout->getDescriptorSetLayout());

	return layouts;
}

void MainMeshRenderSystem::renderModel(VkCommandBuffer cmd, FrameContext& frameContext, const ModelAsset& model, uint32_t lodIndex, glm::mat4 modelMat, glm::mat4 normalM) const
{
	drawModel(cmd, model, frameContext, lodIndex, frameContext.frameIndex, modelMat, normalM);
}

void MainMeshRenderSystem::bindModelDescriptors(VkCommandBuffer cmd, const ModelLOD& model, uint32_t materialIndex, uint32_t frameIndex) const
{
	model.materials[materialIndex].descriptorSet;

}

void MainMeshRenderSystem::drawModel(VkCommandBuffer cmd, const ModelAsset& model, FrameContext& frameContext, uint32_t lodIndex, uint32_t frameIndex, glm::mat4 modelMat, glm::mat4 normalM) const
{
	for (auto& node : model.lods[lodIndex].nodes) {
		drawNode(cmd, model.lods[lodIndex], node, static_cast<uint16_t>(frameIndex), modelMat, normalM);
	}
}

void MainMeshRenderSystem::drawNode(VkCommandBuffer& commandBuffer,
	const ModelLOD& model, Node* node, uint16_t frameIndex, 
	glm::mat4 modelMat, glm::mat4 normalM) const
{

	for (auto& child : node->children) {

		drawNode(commandBuffer, model, child, frameIndex, modelMat, normalM);
	}

	for (auto primitive : node->primitives)
	{
		bindModelDescriptors(commandBuffer, model, primitive.materialIndex, frameIndex);
		drawPrimitive(commandBuffer, primitive, modelMat, normalM);
	}
}


void MainMeshRenderSystem::drawPrimitive(VkCommandBuffer& commandBuffer, Primitive& primitive, glm::mat4 modelMat, glm::mat4 normalM) const
{
	struct { glm::mat4 model; glm::mat4 normal; } push{};
	push.model = modelMat;
	push.normal = normalM;

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(push),
		&push
	);

	vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

}
