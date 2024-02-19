#include "point_light_system.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <map>

struct PointLightPushConstants {
	glm::vec4 position{};
	glm::vec4 color{};
	float radius;
};

PointLightSystem::PointLightSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{ device }
{
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstants);

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

void PointLightSystem::createPipeline(VkRenderPass renderPass)
{
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	Pipeline::enableAlphaBlending(pipelineConfig);
	pipelineConfig.bindingDescription.clear();
	pipelineConfig.attributeDescription.clear();

	pipelineConfig.renderPass = renderPass;

	pipelineConfig.pipelineLayout = pipelineLayout;

	pipeline = std::make_unique<Pipeline>(
		device,
		"point_light.vert.spv",
		"point_light.frag.spv",
		pipelineConfig
	);
}

void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo)
{
	auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, { 0.f, -1.0f, 0.f });


	int lightIndex = 0;
	for (auto& kv : frameInfo.gameObjects) {
		auto& obj = kv.second;
		if (obj.pointLight == nullptr) continue;

		assert(lightIndex < MAX_LIGHT && "point lights exceed maximum");

		// update light position:
		obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

		// copy light to ubo
		ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
		ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->LightIntencity);
		lightIndex++;
	}
	ubo.numLights = lightIndex;
}

void PointLightSystem::render(FrameInfo& frameInfo)
{
	// sort lights
	std::map<float, GameObject::id_t> sorted;
	for (auto& kv : frameInfo.gameObjects) {
		auto& obj = kv.second;
		if (obj.pointLight == nullptr) continue;

		// get dist
		auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
		float disSquared = glm::dot(offset, offset);
		sorted[disSquared] = obj.getId();
	}

	pipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	// iterate through sorted map in inverse order:
	for (auto it = sorted.rbegin(); it != sorted.rend(); it++) {
		auto& obj = frameInfo.gameObjects.at(it->second);

		PointLightPushConstants push{};
		push.position = glm::vec4(obj.transform.translation, 1.f);
		push.color = glm::vec4(obj.color, obj.pointLight->LightIntencity);
		push.radius = obj.transform.scale.x;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PointLightPushConstants),
			&push
		);
		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}

	
}
