#include "GLTFrenderSystem.h"

#include "GLTFrenderSystem.h"


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

GlTFrenderSystem::GlTFrenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{ device }
{

	//// GlTf pipeline
	createPipelineLayout({ globalSetLayout, (*DescriptorSetLayout::Builder(device)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build())
		.getDescriptorSetLayout() });

	createPipelineGlTf(renderPass);
}

GlTFrenderSystem::~GlTFrenderSystem()
{
	vkDestroyPipelineLayout(device.device(), GlTFPipelineLayout, nullptr);
}

void GlTFrenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
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

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &GlTFPipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void GlTFrenderSystem::createPipelineGlTf(VkRenderPass renderPass)
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

void GlTFrenderSystem::renderGlTFModel(FrameInfo& frameInfo, GameObject& obj)
{
	if (obj.gltfModel != nullptr)
	{

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GlTFPipelineLayout,
			1, 1,
			&obj.gltfModel->descriptorSet[frameInfo.frameIndex],
			0,
			nullptr
		);

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
		obj.gltfModel->draw(frameInfo.commandBuffer, GlTFPipelineLayout);
	}
}



void GlTFrenderSystem::renderGameObjects(FrameInfo& frameInfo)
{
	GlTFPipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		GlTFPipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet[frameInfo.frameIndex],
		0,
		nullptr
	);


	int i = 0;
	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;

		if (obj.gltfModel != nullptr)
			renderGlTFModel(frameInfo, obj);

		i++;
	}
}
