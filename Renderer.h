#pragma once

#include "Window.h"

#include "device.h"
#include "Swap_chain.h"
#include "DepthSwapChain.h"
#include "Frame_info.h"
#include "GlobalRenderSystem.h"

#include <memory>
#include <vector>
#include <cassert>



class Renderer
{
public:

	Renderer(Window& window, Device& device);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
	VkRenderPass getDepthRenderPass() const { return depthSwapChain->getDepthRenderPass(); }
	float getAspectRatio() const { return swapChain->extentAspectRatio(); }

	uint32_t getWidth() const { return swapChain->width(); }
	uint32_t getHeight() const { return swapChain->height(); }

	bool isFrameInProgress() const { return isFrameStarted;	}

	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted && "cannot get command buffer when frame not in progress");
		return commandBuffers[currentFrameIndex];
	}
	
	VkCommandBuffer getCurrentDepthCommandBuffer() const {
		assert(isDepthStarted && "cannot get command buffer when frame not in progress");
		return depthCommandBuffers[currentDepthIndex];
	}

	int getFrameIndex() const {
		//assert(isFrameStarted && "cannot get frame index when frame not in progress");
		return currentFrameIndex;
	}

	int getDepthIndex() const {
		assert(isDepthStarted && "cannot get frame index when frame not in progress");
		return currentDepthIndex;
	}

	VkCommandBuffer beginFrame();
	void endFrame();

	VkCommandBuffer beginDepthFrame();
	void endDepthFrame();

	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	void beginShadowRenderPass(VkCommandBuffer commandBuffer);
	void endShadowRenderPass(VkCommandBuffer commandBuffer);

	void submitCommandBuffers(bool renderDepth);

	bool aquireNextImage();

	VkDescriptorImageInfo getShadowImageInfo(int i) { return depthSwapChain->getShadowImageInfo(i); }

	void renderDepthImage(FrameInfo& frameInfo, GlobalRenderSystem& renderSystems);
	

private:

	void createCommandBuffer();
	void createDepthCommandBuffer();
	void freeCommandBuffers();
	void recreateSwapChain();

	void transitionDepthImageLayout(VkCommandBuffer& commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

	Window& window;
	Device& device;

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<DepthSwapChain> depthSwapChain;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> depthCommandBuffers;

	uint32_t currentImageIndex;
	uint32_t currentDepthImageIndex;

	int currentFrameIndex;
	int currentDepthIndex;

	bool isFrameStarted = false;
	bool isDepthStarted = false;
};

