#pragma once

#include "BaseRenderPass.h"
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include "../base/Swap_chain.h"
class Device;
class AssetManager;

class ColorPass: public BaseRenderPass
{
public:
	ColorPass(Device& device_, AssetManager& assets_, Swap_chain* swapchain)
		: BaseRenderPass(device_, assets_) {
		createRenderPass(
			swapchain->getSwapChainImageFormat(),
			swapchain->getSwapChainDepthFormat()
		);
	}

	void createRenderSystems() {};
	void recordPass()  {};
	void createRenderPass(VkFormat imageFormat, VkFormat depthFormat);

	void beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex);
	void endRenderPass(VkCommandBuffer commandBuffer);

private:
};
