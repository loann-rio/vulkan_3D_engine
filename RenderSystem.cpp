#include "RenderSystem.h"


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

RenderSystem::RenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device}
{

	//// Obj pipeline
	createPipelineLayout({ globalSetLayout, (*DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build())
		.getDescriptorSetLayout() }, objPipelineLayout);

	createPipeline(renderPass);



	//// GlTf pipeline
	createPipelineLayout({ globalSetLayout }, GlTFPipelineLayout);

	createPipelineGlTf(renderPass);


}

RenderSystem::~RenderSystem()
{
	vkDestroyPipelineLayout(device.device(), objPipelineLayout , nullptr);
	vkDestroyPipelineLayout(device.device(), GlTFPipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout, VkPipelineLayout & pipelineLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	VkDescriptorBindingFlags descriptorBindingFlags[] = {
	0,  // For non-dynamic descriptor sets
	VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT // For dynamic descriptor sets
	};


	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo = {};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	bindingFlagsInfo.bindingCount = sizeof(descriptorBindingFlags) / sizeof(descriptorBindingFlags[0]);
	bindingFlagsInfo.pBindingFlags = descriptorBindingFlags;
	

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutInfo.pNext = &bindingFlagsInfo;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void RenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(objPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.renderPass = renderPass;

	pipelineConfig.pipelineLayout = objPipelineLayout;

	objPipeline = std::make_unique<Pipeline>(
		device,
		"simple_shader.vert.spv",
		"simple_shader.frag.spv",
		pipelineConfig
	);
}

void RenderSystem::createPipelineGlTf(VkRenderPass renderPass)
{
	assert(GlTFPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.bindingDescription = GlTFModel::ModelGltf::Vertex::getBindingDescriptionsGlTF();
	pipelineConfig.attributeDescription = GlTFModel::ModelGltf::Vertex::getAttributeDescriptionsGlTF();

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = GlTFPipelineLayout;

	GlTFPipeline = std::make_unique<Pipeline>(
		device,
		"GlTFshader.vert.spv",
		"GlTFshader.frag.spv",
		pipelineConfig
	);
}

void RenderSystem::renderObjModel(FrameInfo& frameInfo, GameObject& obj)
{
	if (obj.model != nullptr)
	{
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			objPipelineLayout,
			1, 1,
			&obj.model->getDecriptorSet()[frameInfo.frameIndex],
			0,
			nullptr
		);

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

		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer);
	}
}

void RenderSystem::renderGlTFModel(FrameInfo& frameInfo, GameObject& obj)
{
	if (obj.gltfModel != nullptr)
	{

		SimplePushConstantData push{};
		push.modelMatrix = obj.transform.mat4();
		push.normalMatrix = obj.transform.normalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			GlTFPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push
		);

		obj.gltfModel->bind(frameInfo.commandBuffer);
		obj.gltfModel->draw(frameInfo.commandBuffer);
	}
}

void RenderSystem::renderGameObjects(FrameInfo& frameInfo)
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

	
	int i = 0;
	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;

		if (obj.model != nullptr)
			renderObjModel(frameInfo, obj);
		i++;
	}
}
