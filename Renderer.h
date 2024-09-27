#pragma once

#include "Window.h"

#include "device.h"
#include "Swap_chain.h"

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
	float getAspectRatio() const { return swapChain->extentAspectRatio(); }

	uint32_t getWidth() const { return swapChain->width(); }
	uint32_t getHeight() const { return swapChain->height(); }

	bool isFrameInProgress() const { return isFrameStarted;	}

	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted && "cannot get command buffer when frame not in progress");
		return commandBuffers[currentFrameIndex];
	}

	int getFrameIndex() const {
		assert(isFrameStarted && "cannot get frame index when frame not in progress");
		return currentFrameIndex;
	}

	VkCommandBuffer beginFrame();
	void endFrame();

	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);


private:

	void createCommandBuffer();
	void freeCommandBuffer();
	void recreateSwapChain();

	Window& window;
	Device& device;

	std::unique_ptr<Swap_chain> swapChain;
	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentImageIndex;
	int currentFrameIndex;
	bool isFrameStarted = false;
};

