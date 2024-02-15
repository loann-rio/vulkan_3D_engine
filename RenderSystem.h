#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"

#include <memory>
#include <vector>

template<class T>
constexpr T pi = T(3.1415926535897932385L);

class RenderSystem
{
public:
	RenderSystem(Device& device, VkRenderPass renderPass);
	~RenderSystem();

	RenderSystem(const RenderSystem&) = delete;
	RenderSystem& operator=(const RenderSystem&) = delete;
	void renderGameObjects(Frame_info::FrameInfo& frameInfo, std::vector<GameObject> & gameObjects);


private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);

	Device &device;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
};

