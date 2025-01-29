#include "GlobalRenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>


struct SimplePushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	glm::mat4 normalMatrix{ 1.f };
};

GlobalRenderSystem::GlobalRenderSystem(Device& device, VkRenderPass renderPass, 
	VkDescriptorSetLayout globalSetLayout, std::vector<VkDescriptorType> bindings, 
	const std::string& vertFilepath, const std::string& fragFilepath,
	ModelType modelType,
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription, bool isShadow)
	: device{ device }, modelType{ modelType }, isShadow{ isShadow }   
{
	auto builder = DescriptorSetLayout::Builder(device);

	for (int i = 0; i < bindings.size(); i++) {
		builder.addBinding(i + 1, bindings[i], VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	createPipelineLayout({ globalSetLayout, (*builder.build()).getDescriptorSetLayout() });
	createPipeline(renderPass, vertFilepath, fragFilepath, bindingDescription, attributeDescription);
}

GlobalRenderSystem::~GlobalRenderSystem()
{
	vkDestroyPipelineLayout(device.device(), objPipelineLayout, nullptr);
}

void GlobalRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

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

	objPipeline = std::make_unique<Pipeline>(
		device,
		vertFilepath,
		fragFilepath,
		pipelineConfig
	);
}

void GlobalRenderSystem::renderModel(FrameInfo& frameInfo, GameObject& obj)
{

	if (!isShadow) {
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			objPipelineLayout,
			1, 1,
			&obj.getDescriptorSets()[frameInfo.frameIndex],
			0,
			nullptr
		);
	}

	SimplePushConstantData push{};
	push.modelMatrix = obj.transform.mat4();
	push.normalMatrix = obj.transform.normalMatrix();

	vkCmdPushConstants(
		frameInfo.commandBuffer,
		objPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(SimplePushConstantData),
		&push
	);

	obj.bindModel(frameInfo.commandBuffer);
	obj.drawModel(frameInfo.commandBuffer, objPipelineLayout);
	
}

void GlobalRenderSystem::renderGameObjects(FrameInfo& frameInfo)
{
	objPipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		objPipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet[frameInfo.frameIndex],
		0,
		nullptr
	);

	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		if (obj.modelType == modelType)
			renderModel(frameInfo, obj);
	}
}

