#include "GlobalRenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>


GlobalRenderSystem::GlobalRenderSystem(Device& device, VkRenderPass renderPass, 
	std::vector<VkDescriptorSetLayout> globalSetLayout, std::vector<DescriptorSetObject> bindings,
	const std::string& vertFilepath, const std::string& fragFilepath,
	ModelType modelType, ModelSubType subModelType,
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription, bool isShadow, bool isSkyBox)
	: device{ device }, modelType{ modelType }, isShadow{ isShadow }, modelSubType{ subModelType }, isSkyBox{ isSkyBox }
{
	std::vector<std::unique_ptr<DescriptorSetLayout>> layouts;  // to hold the unique ptr of the descriptor set layouts

	modelDescriptorSetIndex = static_cast<uint32_t>(globalSetLayout.size()); // index of the model descriptor set in the pipeline layout

	// create descriptor set layout for each binding set of the model
	for (size_t j = 0; j < bindings.size(); j++) {
		auto builder = DescriptorSetLayout::Builder(device);

		// add binding for each descriptor in the set
		for (int i = 0; i < bindings[j].descriptorSet.size(); i++) {
			builder.addBinding(i + 1, bindings[j].descriptorSet[i].descriptorType, bindings[j].descriptorSet[i].stage, bindings[j].descriptorSet[i].count);
		}

		auto newLayout = builder.build(); 

		// check if layout creation success
		if (!newLayout) { 
			std::cerr << "Failed to build descriptor set layout at index " << j << "\n"; 
			continue;
		}

		// add to the list of descriptor set layout
		globalSetLayout.push_back(newLayout->getDescriptorSetLayout());
		layouts.push_back(std::move(newLayout));
	}
	
	// create pipelinelayout with the global and model descriptor set layout
	createPipelineLayout(globalSetLayout);

	// create pipeline
	createPipeline(renderPass, vertFilepath, fragFilepath, bindingDescription, attributeDescription);
}

/// <summary>
/// destroy pipelinelayout at removal of object
/// </summary>
GlobalRenderSystem::~GlobalRenderSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void GlobalRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
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
			
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantRange.size = sizeof(SimplePushConstantData);
		}
	}

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

void GlobalRenderSystem::createPipeline(VkRenderPass renderPass, const std::string& vertFilepath, const std::string& fragFilepath, 
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

	if (!attributeDescription.empty()) {
		pipelineConfig.attributeDescription = attributeDescription;
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

	// create the pipeline
	pipeline = std::make_unique<Pipeline>(
		device,
		vertFilepath,
		fragFilepath,
		pipelineConfig
	);
}

void GlobalRenderSystem::renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj, const std::array<FrustumPlane, 6>& frustrumPlanes)
{

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		modelDescriptorSetIndex, 1,
		&obj->getDescriptorSets()[frameInfo.frameIndex],
		0,
		nullptr
	);
	
	obj->bindModel(commandBuffer);

	obj->drawModel(commandBuffer, pipelineLayout, frameInfo.frameIndex, frustrumPlanes);
}

void GlobalRenderSystem::renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex, uint16_t frameIndex)
{
	obj->bindModel(commandBuffer); 
	obj->drawModelDepth(commandBuffer, pipelineLayout, lightIndex, frameIndex);
}

void GlobalRenderSystem::bind(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets)
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

void GlobalRenderSystem::renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, const std::array<FrustumPlane, 6>& frustrumPlanes)
{
	bind(commandBuffer, globalDescriptorSets);
	
	for (auto& obj : frameInfo.listGameObjects)
	{
		if (obj->show && !obj->toBeRemoved && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)

			renderModel(commandBuffer, frameInfo, obj, frustrumPlanes);
	}
}

void GlobalRenderSystem::renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex)
{ 
	// bind pipeline and global descriptor sets
	bind(commandBuffer, globalDescriptorSets);

	// render each model with the corresponding type
	for (auto obj : frameInfo.listGameObjects)
	{
		if (obj->show && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)
			renderModelDepth(commandBuffer, obj, lightIndex, frameIndex);
	}
}
