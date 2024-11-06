#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"
#include "descriptors.h"
#include "Swap_chain.h"

#include <memory>
#include <vector>

template<class T>
constexpr T pi = T(3.1415926535897932385L);

class GlTFrenderSystem
{
public:
	GlTFrenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~GlTFrenderSystem();

	GlTFrenderSystem(const GlTFrenderSystem&) = delete;
	GlTFrenderSystem& operator=(const GlTFrenderSystem&) = delete;

	void renderGameObjects(FrameInfo& frameInfo);

private:
	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipelineGlTf(VkRenderPass renderPass);

	void renderGlTFModel(FrameInfo& frameInfo, GameObject& obj);

	Device& device;

	std::unique_ptr<Pipeline> GlTFPipeline;
	VkPipelineLayout GlTFPipelineLayout;
};

