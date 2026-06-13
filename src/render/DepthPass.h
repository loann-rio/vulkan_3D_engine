#pragma once

#include "BaseRenderPass.h"
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include "GlobalRenderSystem.h"

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

	void createRenderPass(VkFormat imageFormat, VkFormat depthFormat);

	void beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex);
	void endRenderPass(VkCommandBuffer commandBuffer);

private:
	void createRenderSystems();

	std::shared_ptr<GlobalRenderSystem> depthRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystemGltf;
	std::shared_ptr<GlobalRenderSystem> depthTerrainRenderSystem;
};