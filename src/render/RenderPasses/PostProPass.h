#pragma once

#include "BaseRenderPass.h"
#include "../../base/Swap_chain.h"
#include "../../objects/BasicUI.h"
#include "../RenderSystem.h"

#include <vulkan/vulkan_core.h>


class Device;
class AssetManager;

class PostProPass : public BaseRenderPass
{
public:
	PostProPass(Device& device_, AssetManager& assets_, Swap_chain* swapchain)
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
	) override;

private:
	void createRenderSystems();

	void createRenderPass(
		VkFormat imageFormat,
		VkFormat depthFormat
	) override;

	void beginRenderPass(
		VkCommandBuffer commandBuffer,
		int depthRenderIndex,
		int frameIndex
	) override;

	void endRenderPass(
		VkCommandBuffer commandBuffer
	) override;

	std::shared_ptr<RenderSystem> postProcessingRenderSystem;
};
