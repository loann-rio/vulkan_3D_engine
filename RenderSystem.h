#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"
#include "descriptors.h"


#include <memory>
#include <vector>

class RenderSystem
{
public:
	RenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~RenderSystem();

	RenderSystem(const RenderSystem&) = delete;
	RenderSystem& operator=(const RenderSystem&) = delete;
	void renderGameObjects(FrameInfo& frameInfo);


private:
	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout, VkPipelineLayout& pipelineLayout);
	void createPipeline(VkRenderPass renderPass);
	void createPipelineGlTf(VkRenderPass renderPass);

	void renderObjModel(FrameInfo& frameInfo, GameObject& obj);
	void renderGlTFModel(FrameInfo& frameInfo, GameObject& obj);

	Device &device;

	std::unique_ptr<Pipeline> objPipeline;
	VkPipelineLayout objPipelineLayout;

	std::unique_ptr<Pipeline> GlTFPipeline;
	VkPipelineLayout GlTFPipelineLayout;
};

