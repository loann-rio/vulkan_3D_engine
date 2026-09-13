#pragma once

#include "BaseRenderPass.h"
#include "../RenderSystem.h"

#include <vulkan/vulkan_core.h>
#include <cstdint>

class Device;
class AssetManager;

class DepthPass : public BaseRenderPass
{
public:

	static const uint16_t MAX_DEPTH_RENDER_COUNT = 4;

	DepthPass(Device& device_, AssetManager& assets_, Swap_chain* swapchain)
		: BaseRenderPass(device_, assets_) {

		createRenderPass(
			swapchain->getSwapChainImageFormat(),
			swapchain->getSwapChainDepthFormat()
		);

		createRenderSystems();
	}

	void recordPass(
		ObjectManager& objectManager,
		FrameInfo& frameInfo,
		VkCommandBuffer& commandBuffer
	);

private:
	void createRenderSystems();

	void createRenderPass(VkFormat imageFormat, VkFormat depthFormat);

	void beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex);
	void endRenderPass(VkCommandBuffer commandBuffer);

	std::shared_ptr<RenderSystem> depthRenderSystem;
	std::shared_ptr<RenderSystem> depthRenderSystemGltf;
	std::shared_ptr<RenderSystem> depthTerrainRenderSystem;
};