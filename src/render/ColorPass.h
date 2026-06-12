#pragma once

#include "BaseRenderPass.h"
#include "../base/Swap_chain.h"
#include "../objects/BasicUI.h"
#include "GlobalRenderSystem.h"

#include <vulkan/vulkan_core.h>


class Device;
class AssetManager;

class ColorPass : public BaseRenderPass
{
public:
	ColorPass(Device& device_, AssetManager& assets_, Swap_chain* swapchain)
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

	void setUi(
		BasicUI* ui
	) {
		imgui = ui;
	}

private:
	void createRenderSystems();

	std::shared_ptr<GlobalRenderSystem> gltfRenderSystem;
	std::shared_ptr<GlobalRenderSystem> objRenderSystem;
	std::shared_ptr<GlobalRenderSystem> terrainRenderSystem;
	std::shared_ptr<GlobalRenderSystem> skyboxRenderSystem;

	BasicUI* imgui;
};
