#pragma once

#include "../base/Window.h"
#include "../base/device.h"
#include "../base/Swap_chain.h"
#include "../base/DepthSwapChain.h"
#include "../base/SingleRenderSwap.h"
#include "../base/FrameRateCounter.h"
#include "../base/Frame_info.h"

#include "../assetManager/AssetManager.h"

#include "../objects/objectManager.h"
#include "../objects/BasicUI.h"

#include "FrameRenderer.h"

#include "GlobalRenderSystem.h"
#include "PassTarget.h"

#include <memory>
#include <vector>

class Renderer
{
public:

	Renderer(Window& window, Device& device, AssetManager& assets, ObjectManager& objectManager);

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
	VkRenderPass getDepthRenderPass() const { return depthSwapChain->getDepthRenderPass(); }
	VkRenderPass getSecondarySwapRenderPass() const { return skyboxSwapChain.getRenderPass(); }
	
	float getAspectRatio() const { return swapChain->extentAspectRatio(); }
	uint32_t getWidth() const { return swapChain->width(); }
	uint32_t getHeight() const { return swapChain->height(); }

	uint32_t getFrameIndex() const { return frameRenderer.getCurrentFrameIndex(); }


	VkDescriptorImageInfo getDepthImageInfo(uint16_t index) { return depthFrameTarget->getDepthImageInfo(index); }

	void generateSkybox(const std::string pathTexture, const std::string goName, ObjectManager& objectManager);

	void renderFrame(FrameInfo& frameInfo, ObjectManager& objectManager);
	bool aquireNextImage();

	bool isUiSelected() { return imgui->isWindowSelected; }

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;
	
private:
	void recreateSwapChain();
	void createRenderSystems(ObjectManager& objectManager);
	void createTextureTarget(ObjectManager& objectManager);

	void beginSingleTimeRender(VkCommandBuffer commandBuffer, int buffer_index = 0);
	void beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex);
	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent);

	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	void renderColorImage(ObjectManager& objectManager, FrameInfo& frameInfo, VkCommandBuffer& commandBuffer);
	void renderDepthImage(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer);

	TextureManager::TextureID renderHdriToCubeTexture(std::shared_ptr<GlobalRenderSystem> renderSystem, VkDescriptorSet descriptorSet);

	Window& window;
	Device& device;
	AssetManager& assets; 
	GameObjectModel* base_skybox;
	FrameRenderer frameRenderer{ device, swapChain.get()};;

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<DepthSwapChain> depthSwapChain;

	SingleSwapChain skyboxSwapChain{ device, assets, {2000, 2000} };

	std::unique_ptr<BasicUI> imgui;

	// target
	std::unique_ptr<PassTarget> depthFrameTarget;
	std::unique_ptr<PassTarget> finalFrameTarget;

	// fps
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

