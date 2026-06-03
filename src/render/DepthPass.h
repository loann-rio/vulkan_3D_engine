#pragma once

#include "BaseRenderPass.h"
#include <vulkan/vulkan_core.h>
#include <cstdint>

class Device;
class AssetManager;

class DepthPass : public BaseRenderPass
{
	static const uint16_t MAX_DEPTH_RENDER_COUNT = 4;
public:
	DepthPass(Device& device_, AssetManager& assets_, Swap_chain* swapchain)
		: BaseRenderPass(device_, assets_) {}

	void createRenderSystems() {};
	void recordPass() {};
	void createRenderPass(VkFormat depthFormat);

	void beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex);
	void endRenderPass(VkCommandBuffer commandBuffer);

private:
};