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
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

RenderSystem::~RenderSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayout{ globalSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void RenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.renderPass = renderPass;

	pipelineConfig.pipelineLayout = pipelineLayout; 

	pipeline = std::make_unique<Pipeline>(
		device,
		"simple_shader.vert.spv",
		"simple_shader.frag.spv",
		pipelineConfig
	);
}

void RenderSystem::renderGameObjects(FrameInfo& frameInfo, DescriptorSetLayout& setLayout, DescriptorPool& pool)
{
	pipeline->bind(frameInfo.commandBuffer);

	//DescriptorWriter(setLayout, pool)
	//			//.writeImage(1, &imageInfo)
	//			.overwrite(frameInfo.globalDescriptorSet);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	for (auto& kv : frameInfo.gameObjects)
	{
		auto& obj = kv.second;
		if (obj.model == nullptr) continue;

		SimplePushConstantData push{};
		push.modelMatrix = obj.transform.mat4();
		push.normalMatrix = obj.transform.normalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push
		);
		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer);
	}
}
