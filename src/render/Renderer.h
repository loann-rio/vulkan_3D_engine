#pragma once

#include "../base/Window.h"
#include "../base/Swap_chain.h"
#include "../base/SingleRenderSwap.h"
#include "../base/FrameRateCounter.h"
#include "../base/Frame_info.h"

#include "../assetManager/AssetManager.h"

#include "../objects/objectManager.h"
#include "../objects/BasicUI.h"


#include "RenderPasses/DepthPass.h"
#include "RenderPasses/ColorPass.h"
#include "RenderPasses/PostProPass.h"

#include "RenderSystem.h"
#include "FrameRenderer.h"
#include "PassTarget.h"

#include <memory>
#include <vector>

class Device;

class Renderer
{
public:

	Renderer(Window& window, Device& device, AssetManager& assets, ObjectManager& objectManager);

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	
	float getAspectRatio() const { return swapChain->extentAspectRatio(); }
	uint32_t getFrameIndex() const { return frameRenderer.getCurrentFrameIndex(); }

	void generateSkybox(const std::string pathTexture, const std::string goName, ObjectManager& objectManager);

	void renderFrame(FrameInfo& frameInfo, ObjectManager& objectManager);

	bool isUiSelected() { return imgui->isWindowSelected; }

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;
	
private:
	void recreateSwapChain(ObjectManager& objectManager);
	void createBuffers(ObjectManager& objectManager);
	void createTextureTarget();
	void createPasses();
	void initUi();

	bool aquireNextImage(ObjectManager& objectManager);

	void beginSingleTimeRender(VkCommandBuffer commandBuffer, int buffer_index = 0);
	TextureManager::TextureID renderHdriToCubeTexture(std::shared_ptr<RenderSystem> renderSystem, VkDescriptorSet descriptorSet);

	Window& window;
	Device& device;
	AssetManager& assets; 
	GameObjectModel* base_skybox;
	std::unique_ptr<Swap_chain> swapChain;
	FrameRenderer frameRenderer{ device, swapChain.get()};;

	SingleSwapChain skyboxSwapChain{ device, assets, {2000, 2000} };

	std::unique_ptr<BasicUI> imgui;

	// target
	std::unique_ptr<PassTarget> depthFrameTarget;
	std::unique_ptr<PassTarget> colorFrameTarget;
	std::unique_ptr<PassTarget> postPFrameTarget;

	// fps
	float gpuTime = 0.0f;
	FrameRateCounter gpuFrameRate;

	// passes
	std::unique_ptr<DepthPass> depthPass;
	std::unique_ptr<ColorPass> colorPass;
	std::unique_ptr<PostProPass> postPass;

	// render systems
	std::shared_ptr<RenderSystem> skyboxCreationRenderSystem;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> postProDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;
};

