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
	
	VkCommandBuffer getCurrentDepthCommandBuffer(int depthCommandBufferIndex) const {
		assert(isDepthStarted[depthCommandBufferIndex] && "cannot get command buffer when frame not in progress");
		//std::cout << "get depth command buffer index " << depthCommandBufferIndex + currentDepthFrameIndex * Swap_chain::MAX_FRAMES_IN_FLIGHT << "\n";
		return depthCommandBuffers[depthCommandBufferIndex + currentDepthFrameIndex * Swap_chain::MAX_FRAMES_IN_FLIGHT];
	}

	std::vector<VkCommandBuffer> getCurrentDepthCommandBuffers() const {
		//std::cout << "get depth command buffer index from" << currentDepthFrameIndex * Swap_chain::MAX_FRAMES_IN_FLIGHT << " to " << DepthSwapChain::MAX_DEPTH_RENDER_COUNT * (currentDepthFrameIndex + 1) << "\n";
		//assert(std::all_of(isDepthStarted.begin(), isDepthStarted.end(), [](bool v) { return v; }) && "cannot get command buffer when frame not in progress");
		return {depthCommandBuffers.begin() + DepthSwapChain::MAX_DEPTH_RENDER_COUNT * currentDepthFrameIndex, depthCommandBuffers.begin() + DepthSwapChain::MAX_DEPTH_RENDER_COUNT * (currentDepthFrameIndex + 1 )}; //depthCommandBuffers;
	}

	int getFrameIndex() const {
		//assert(isFrameStarted && "cannot get frame index when frame not in progress"); 
		return currentFrameIndex;
	}

	int getDepthIndex() const {
		return currentDepthFrameIndex;
	}

	VkCommandBuffer beginFrame();
	void endFrame();

	VkCommandBuffer beginDepthFrame(int depthCommandBufferIndex);
	void endDepthFrame(int depthCommandBufferIndex);

	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	void beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex);
	void endShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex);

	void submitCommandBuffers(bool renderDepth);
	bool aquireNextImage();

	VkDescriptorImageInfo* getShadowImageInfo(int i) { return depthSwapChain->getShadowImageInfo(i); } 

	void renderDepthImage(FrameInfo& frameInfo, std::shared_ptr<GlobalRenderSystem> renderSystems);
	
private:

	void createCommandBuffer();
	void createDepthCommandBuffer();
	void freeCommandBuffers();
	void recreateSwapChain();

	Window& window;
	Device& device;

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<DepthSwapChain> depthSwapChain;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> depthCommandBuffers;

	uint32_t currentImageIndex;
	uint32_t currentDepthImageIndex;

	int currentFrameIndex;
	int currentDepthFrameIndex;

	bool isFrameStarted = false; 
	std::vector<bool> isDepthStarted;
};

