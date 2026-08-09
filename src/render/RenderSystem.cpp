#include "RenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>

RenderSystem::RenderSystem(Device& device, AssetManager& assets, RenderSystemConfig config):
	device{ device },
	assets{ assets },
	modelType{ config.modelType },
	modelSubType{ config.modelSubType },
	isShadow{ config.shadow },
	isSkyBox{ config.skybox },
	isFullscreenRender{ config.fullscreen }, 
	modelDescriptorSetIndex{ config.modelDescriptorSetIndex },
	customPushStage{ config.pushStage != 0 },
	pushStage{ static_cast<VkShaderStageFlagBits>(config.pushStage) }
{
	// Build descriptor set layouts for model descriptorBindings and append to the global layouts
	std::vector<std::unique_ptr<DescriptorSetLayout>> layouts;
	// copy global layouts so we can append model layouts before creating pipeline layout
	std::vector<VkDescriptorSetLayout> layoutsToCreate = config.globalLayouts;

	for (size_t j = 0; j < config.descriptorBindings.size(); ++j) {
		auto builder = DescriptorSetLayout::Builder(device);
		for (int i = 0; i < config.descriptorBindings[j].descriptorSet.size(); ++i) {
			const auto& desc = config.descriptorBindings[j].descriptorSet[i];
			builder.addBinding(i, desc.descriptorType, desc.stage, desc.count);
		}
		auto newLayout = builder.build();
		if (!newLayout) {
			std::cerr << "Failed to build descriptor set layout at index " << j << "\n";
			continue;
		}
		layoutsToCreate.push_back(newLayout->getDescriptorSetLayout());
		layouts.push_back(std::move(newLayout));
	}

	// create pipelinelayout with the global and model descriptor set layout
	createPipelineLayout(
		layoutsToCreate//config.globalLayouts
	);

	// create pipeline
	createPipeline(
		config.renderPass, 
		config.vertexShader, 
		config.fragmentShader, 
		config.bindingDescriptions,
		config.attributeDescriptions
	);
}

/// <summary>
/// destroy pipelinelayout at removal of object
/// </summary>
RenderSystem::~RenderSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;


	// find the size and flag of push constant depending on type of obj/render
	
	if (isShadow) { 
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.size = sizeof(DepthPushConstantData);
	}

	else {
		if (modelType == ModelType::GLTF_MODEL) {
			// gltf model need the push constant both in vertex and frag shader 
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantRange.size = sizeof(GltfPushConstant); 
		}
		else {
			pushConstantRange.stageFlags = isFullscreenRender ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantRange.size = sizeof(SimplePushConstantData);
		}
	}

	if (customPushStage) pushConstantRange.stageFlags = pushStage;

	VkDescriptorBindingFlags descriptorBindingFlags[] = {
	0,  // For non-dynamic descriptor sets
	VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT // For dynamic descriptor sets
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	// create layout
	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void RenderSystem::createPipeline(VkRenderPass renderPass, const std::string& vertFilepath, const std::string& fragFilepath, 
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription)
{
	// check if pipeline layout is created
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	// get default pipeline configuration
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	if (!bindingDescription.empty()) {
		pipelineConfig.bindingDescription = bindingDescription;
	}
	else {
		pipelineConfig.bindingDescription.clear();
	}

	if (!attributeDescription.empty()) {
		pipelineConfig.attributeDescription = attributeDescription;
	}
	else {
		pipelineConfig.attributeDescription.clear();
	}

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	
	// enable depth bias if no frag shader (depth only)
	if (fragFilepath.empty()) {
		pipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
		pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 4.0f;
		pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 1.5f;
	}

	if (isSkyBox) {
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	}

	if (isFullscreenRender) {
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	}

	// create the pipeline
	pipeline = std::make_unique<Pipeline>(
		device,
		vertFilepath,
		fragFilepath,
		pipelineConfig
	);
}

void RenderSystem::renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj, const std::array<FrustumPlane, 6>& frustrumPlanes)
{
	if (!Camera::isAABBinFrustrum(obj->getAABB().getAABB(obj->getTransformMat()), frustrumPlanes)) return;
	
	obj->bindModel(commandBuffer, true, pipelineLayout, frameInfo.frameIndex, modelDescriptorSetIndex);

	obj->drawModel(commandBuffer, pipelineLayout, frameInfo.frameIndex, frustrumPlanes);
}

void RenderSystem::renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex, uint16_t frameIndex, const std::array<FrustumPlane, 6>& planes) 
{
	obj->bindModel(commandBuffer, false, pipelineLayout, 1, 1);
	obj->drawModelDepth(commandBuffer, pipelineLayout, lightIndex, frameIndex, planes);
}

void RenderSystem::bind(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets)
{
	pipeline->bind(commandBuffer);

	for (uint16_t setIndex = 0; setIndex < globalDescriptorSets.size(); setIndex++)
	{
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			setIndex, 1,
			&globalDescriptorSets[setIndex],
			0, nullptr 
		);
	}
}

void RenderSystem::bindModel(VkCommandBuffer& commandBuffer, ModelAsset* model)
{
	VkBuffer buffers[] = { model->lods[0].vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model->lods[0].indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void RenderSystem::bindTextures(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, uint16_t frameIndex)
{
	vkCmdBindDescriptorSets(commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		modelDescriptorSetIndex, 1,
		&model->lods[0].materials[primitive.materialIndex].descriptorSet[frameIndex],
		0,
		nullptr);
}

void RenderSystem::drawModel(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, glm::mat4 modelMat, glm::mat4 normalM)
{
	SimplePushConstantData push{};
	push.modelMatrix = modelMat;
	push.normalMatrix = normalM;

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

void RenderSystem::renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, const std::array<FrustumPlane, 6>& frustrumPlanes)
{
	bind(commandBuffer, globalDescriptorSets);
	
	for (auto& obj : frameInfo.listGameObjects)
	{
		if (obj->show && !obj->toBeRemoved && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)
		{
			

			if (obj->modelAsset) {

				auto modelAsset = assets.models().get(obj->modelAsset);

				bindModel(commandBuffer, modelAsset);

				for (auto primitive : modelAsset->lods[0].primitives)
				{
					bindTextures(commandBuffer, modelAsset, primitive, frameInfo.frameIndex);
					drawModel(commandBuffer, modelAsset, primitive, obj->getTransformMat(), obj->getNormalMat());
				}
			}
			else
			{
				renderModel(commandBuffer, frameInfo, obj, frustrumPlanes);
			}
		}
	}
}

void RenderSystem::renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex)
{ 
	// bind pipeline and global descriptor sets
	bind(commandBuffer, globalDescriptorSets);

	// render each model with the corresponding type
	for (auto obj : frameInfo.listGameObjects)
	{
		if (obj->show && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)
			renderModelDepth(commandBuffer, obj, lightIndex, frameIndex, frameInfo.listFrustrumPlanes[lightIndex]);
	}
}

void RenderSystem::renderFullScreen(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets, glm::mat4 view, glm::mat4 proj)
{

	bind(commandBuffer, globalDescriptorSets);

	struct Push { glm::mat4 view; glm::mat4 proj; } push;

	push.view = view;
	push.proj = proj;

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
	
	// draw fullscreen triangle
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}
