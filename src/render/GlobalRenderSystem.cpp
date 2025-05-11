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
	ModelType modelType,
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription, bool isShadow)
	: device{ device }, modelType{ modelType }, isShadow{ isShadow }   
{
	std::vector<std::unique_ptr<DescriptorSetLayout>> layouts;

	for (size_t j = 0; j < bindings.size(); j++) {
		auto builder = DescriptorSetLayout::Builder(device);

		for (int i = 0; i < bindings[j].descriptorSet.size(); i++) {
			builder.addBinding(i + 1, bindings[j].descriptorSet[i].descriptorType, bindings[j].descriptorSet[i].stage, bindings[j].descriptorSet[i].count);
		}

		auto newLayout = builder.build(); 

		if (!newLayout) { 
			std::cerr << "Failed to build descriptor set layout at index " << j << "\n"; 
			continue;
		} 

		globalSetLayout.push_back(newLayout->getDescriptorSetLayout());  
		layouts.push_back(std::move(newLayout)); 
	}
	
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass, vertFilepath, fragFilepath, bindingDescription, attributeDescription);
}

GlobalRenderSystem::~GlobalRenderSystem()
{
	vkDestroyPipelineLayout(device.device(), objPipelineLayout, nullptr);
}

void GlobalRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;

	if (isShadow) { 
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.size = sizeof(DepthPushConstantData);
	}
	else {
		if (modelType == ModelType::GLTF_MODEL) {
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

	/*VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo = {};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT; 
	bindingFlagsInfo.bindingCount = sizeof(descriptorBindingFlags) / sizeof(descriptorBindingFlags[0]); 
	bindingFlagsInfo.pBindingFlags = descriptorBindingFlags;*/

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	
	//pipelineLayoutInfo.pNext = &bindingFlagsInfo;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &objPipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void GlobalRenderSystem::createPipeline(VkRenderPass renderPass, const std::string& vertFilepath, const std::string& fragFilepath, 
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription)
{
	assert(objPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	if (!bindingDescription.empty()) {
		pipelineConfig.bindingDescription = bindingDescription;
	}

	if (!attributeDescription.empty()) {
		pipelineConfig.attributeDescription = attributeDescription;
	}

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = objPipelineLayout;

	if (fragFilepath.empty()) {
		pipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
		pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 4.0f;
		pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 1.5f;
	}

	objPipeline = std::make_unique<Pipeline>(
		device,
		vertFilepath,
		fragFilepath,
		pipelineConfig
	);
}

void GlobalRenderSystem::renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj)
{

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		objPipelineLayout,
		2, 1,
		&obj->getDescriptorSets()[frameInfo.frameIndex],
		0,
		nullptr
	);

	obj->bindModel(commandBuffer);

	obj->drawModel(commandBuffer, objPipelineLayout);
}

void GlobalRenderSystem::renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex)
{
	obj->bindModel(commandBuffer); 
	obj->drawModelDepth(commandBuffer, objPipelineLayout, lightIndex); 
}

void GlobalRenderSystem::bind(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets)
{

	objPipeline->bind(commandBuffer); 

	for (uint16_t setIndex = 0; setIndex < globalDescriptorSets.size(); setIndex++)
	{
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			objPipelineLayout,
			setIndex, 1,
			&globalDescriptorSets[setIndex],
			0, nullptr 
		);
	}
}

void GlobalRenderSystem::renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets)
{
	bind(commandBuffer, globalDescriptorSets);
	
	for (auto obj : frameInfo.listGameObjects)
	{
		if (obj->getModelType() == modelType)
			renderModel(commandBuffer, frameInfo, obj);	
	}
}

void GlobalRenderSystem::renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex)
{ 
	// bind pipeline and global descriptor sets
	bind(commandBuffer, globalDescriptorSets);

	// render each model with the corresponding type
	for (auto obj : frameInfo.listGameObjects)
	{
		if (obj->getModelType() == modelType)
			renderModelDepth(commandBuffer, obj, lightIndex);
	}
}
