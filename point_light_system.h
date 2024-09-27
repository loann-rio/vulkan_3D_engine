#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"

#include <memory>
#include <vector>

class PointLightSystem
{
public:
	PointLightSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~PointLightSystem();

	PointLightSystem(const PointLightSystem&) = delete;
	PointLightSystem& operator=(const PointLightSystem&) = delete;

	void update(FrameInfo& frameInfo, GlobalUbo& ubo, int frameInd);
	void render(FrameInfo& frameInfo);


private:
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device& device;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
};

