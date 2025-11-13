#pragma once

#include "../base/Window.h"

#include "../base/device.h"

#include "../base/Swap_chain.h"
#include "../base/DepthSwapChain.h"
#include "../base/secondarySwapchain.h"
#include "../base/SingleRenderSwap.h"

#include "../base/Frame_info.h"
#include "GlobalRenderSystem.h"

#include <memory>
#include <vector>
#include <cassert>



class Renderer
{
public:

	std::shared_ptr<Texture> getDepthTexture() { return depthSwapChain->getTexture(0); }
	std::shared_ptr<Texture> getSingleTexture() { return skyboxSwapChain.getTextureColor(); }

	Renderer(Window& window, Device& device);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
	VkRenderPass getDepthRenderPass() const { return depthSwapChain->getDepthRenderPass(); }
	VkRenderPass getSecondarySwapRenderPass() const { return skyboxSwapChain.getRenderPass(); }
	float getAspectRatio() const { return swapChain->extentAspectRatio(); }

	uint32_t getWidth() const { return swapChain->width(); }
	uint32_t getHeight() const { return swapChain->height(); }

	int getFrameIndex() const {	return currentFrameIndex; }
	int getDepthIndex() const {	return currentDepthFrameIndex; }

	VkDescriptorImageInfo* getShadowImageInfo(int i) { return depthSwapChain->getShadowImageInfo(i); }

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	void beginSingleTimeRender(VkCommandBuffer commandBuffer, int buffer_index = 0);
	void endSingleTimeRender(VkCommandBuffer commandBuffer);

	bool aquireNextImage();

	void renderDepthImage(FrameInfo& frameInfo, std::vector<std::shared_ptr<GlobalRenderSystem>> renderSystems, std::vector<VkDescriptorSet> globalDescriptorSets);
	
	std::shared_ptr<Texture> renderHdriToCubeTexture(std::shared_ptr<GlobalRenderSystem> renderSystem, VkDescriptorSet descriptorSet);


	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted && "cannot get command buffer when frame not in progress");
		return commandBuffers[currentFrameIndex];
	}

	VkCommandBuffer getCurrentDepthCommandBuffer(int depthCommandBufferIndex) const {
		assert(isDepthStarted[depthCommandBufferIndex] && "cannot get command buffer when frame not in progress");
		return depthCommandBuffers[depthCommandBufferIndex + currentDepthFrameIndex * DepthSwapChain::MAX_DEPTH_RENDER_COUNT];
	}

	std::vector<VkCommandBuffer> getCurrentDepthCommandBuffers(size_t commandBufferCount) const {
		assert(std::all_of(isDepthStarted.begin(), (isDepthStarted.begin() + commandBufferCount), [](bool v) { return v; }) && "cannot get command buffer when not all frames in progress");
		return { depthCommandBuffers.begin() + DepthSwapChain::MAX_DEPTH_RENDER_COUNT * currentDepthFrameIndex, depthCommandBuffers.begin() + DepthSwapChain::MAX_DEPTH_RENDER_COUNT * currentDepthFrameIndex + commandBufferCount };
	} 
	
private:

	void createCommandBuffer();
	void createDepthCommandBuffer();
	void freeCommandBuffers();
	void recreateSwapChain();

	VkCommandBuffer beginDepthFrame(int depthCommandBufferIndex);
	void endDepthFrame(int depthCommandBufferIndex);
	void beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex);
	void endShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex); 

	Window& window;
	Device& device;

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<DepthSwapChain> depthSwapChain;

	SingleSwapChain skyboxSwapChain{ device, {2000, 2000} };

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> depthCommandBuffers;

	uint32_t currentImageIndex;
	uint32_t currentDepthImageIndex;

	int currentFrameIndex;
	int currentDepthFrameIndex;

	bool isFrameStarted = false; 
	std::vector<bool> isDepthStarted;

};

