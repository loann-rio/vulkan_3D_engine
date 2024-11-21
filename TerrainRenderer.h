#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"
#include "descriptors.h"


#include <memory>
#include <vector>

class TerrainRenderer
{
public:
	TerrainRenderer(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~TerrainRenderer();

	TerrainRenderer(const TerrainRenderer&) = delete;
	TerrainRenderer& operator=(const TerrainRenderer&) = delete;
	void renderTerrain(FrameInfo& frameInfo);


private:
	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device& device;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
};
