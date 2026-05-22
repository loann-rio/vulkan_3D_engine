#pragma once

#include "../base/Window.h"

#include "../base/device.h"

#include "../base/Swap_chain.h"
#include "../base/DepthSwapChain.h"
#include "../base/SingleRenderSwap.h"

#include "../assetManager/AssetManager.h"

#include "../base/FrameRateCounter.h"

#include "../objects/objectManager.h"

// imgui
#include "../objects/BasicUI.h"


#include "../base/Frame_info.h"
#include "GlobalRenderSystem.h"

#include <memory>
#include <vector>
#include <cassert>

class Renderer
{
public:

	TextureManager::TextureID getDepthTexture() { return depthSwapChain->getTexture(0); }
	TextureManager::TextureID getSingleTexture() { return skyboxSwapChain.getTextureColor(); }

	Renderer(Window& window, Device& device, AssetManager& assets, ObjectManager& objectManager);
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

	void renderDepthImage(FrameInfo& frameInfo);

	void renderColorImage(
		ObjectManager& objectManager,
		FrameInfo& frameInfo);

	void generateSkybox(const std::string pathTexture, const std::string goName, ObjectManager& objectManager);

	void renderFrame(FrameInfo& frameInfo, ObjectManager& objectManager);
	
	TextureManager::TextureID renderHdriToCubeTexture(std::shared_ptr<GlobalRenderSystem> renderSystem, VkDescriptorSet descriptorSet);


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

	bool isUiSelected() { return imgui->isWindowSelected; }

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;
	
private:

	void createCommandBuffer();
	void createDepthCommandBuffer();
	void freeCommandBuffers();
	void recreateSwapChain();
	void createRenderSystems(ObjectManager& objectManager);

	VkCommandBuffer beginDepthFrame(int depthCommandBufferIndex);
	void endDepthFrame(int depthCommandBufferIndex);
	void beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex);
	void endShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex); 

	Window& window;
	Device& device;
	AssetManager& assets; 

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<DepthSwapChain> depthSwapChain;

	SingleSwapChain skyboxSwapChain{ device, assets, {2000, 2000} };

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> depthCommandBuffers;

	uint32_t currentImageIndex;
	uint32_t currentDepthImageIndex;

	int currentFrameIndex;
	int currentDepthFrameIndex;

	bool isFrameStarted = false; 
	std::vector<bool> isDepthStarted;

	std::unique_ptr<BasicUI> imgui;

	GameObjectModel* base_skybox;

	float gpuTime = 0.0f;
	FrameRateCounter gpuFrameRate;
	

	// render systems
	std::shared_ptr<GlobalRenderSystem> gltfRenderSystem;
	std::shared_ptr<GlobalRenderSystem> objRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystemGltf;
	std::shared_ptr<GlobalRenderSystem> terrainRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthTerrainRenderSystem;
	std::shared_ptr<GlobalRenderSystem> skyboxRenderSystem;
	std::shared_ptr<GlobalRenderSystem> skyboxCreationRenderSystem;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;
};

