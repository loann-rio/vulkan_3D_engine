/*#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"

#include <memory>
#include <vector>

#define TEXTOVERLAY_MAX_CHAR_COUNT 2048

class TextOverlay
{
public:
	TextOverlay(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~TextOverlay();

	TextOverlay(const TextOverlay&) = delete;
	TextOverlay& operator=(const TextOverlay&) = delete;

	void update(FrameInfo& frameInfo, GlobalUbo& ubo);
	void render(FrameInfo& frameInfo);

private:
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device& device;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
};*/