#pragma once


#include "base/Window.h"
#include "base/device.h"
#include "objects/GameObject.h"

#include "base/descriptors.h"

//#include "render/Renderer.h"
#include "render/GlobalRenderSystem.h"
#include "render/TextOverlay.h"
#include "objects/ObjectManager.h"

#include "assetManager/AssetManager.h"

#include <memory>
#include <vector>


#include "renderSystem/Renderer.h"



class Engine
{
public:
	static constexpr int WIDTH = 1600;
	static constexpr int HEIGHT = 1200;

	Engine();
	~Engine() {};

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();

private:

	void createRenderSystems();

	Window window{ WIDTH, HEIGHT, "vulkan engine" };
	Device device{ window };
	AssetManager assetManager{};
	//Renderer renderer{ window, device, assetManager };
	ObjectManager objectManager{ device, assetManager };

	GlobalRenderer renderer{ device, window, assetManager, objectManager };


	std::unique_ptr<DescriptorPool> globalPool{};

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;
	std::vector<std::unique_ptr<Buffer>> cloudBuffers;



	// render systems
	/*std::shared_ptr<GlobalRenderSystem> gltfRenderSystem;
	std::shared_ptr<GlobalRenderSystem> GlTFAssetRenderSystem;
	std::shared_ptr<GlobalRenderSystem> objRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystemGltf;
	std::shared_ptr<GlobalRenderSystem> terrainRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthTerrainRenderSystem;
	std::shared_ptr<GlobalRenderSystem> skyboxRenderSystem;
	std::shared_ptr<GlobalRenderSystem> skyboxCreationRenderSystem;*/

	//std::shared_ptr<GlobalRenderSystem> cloudRenderSystem;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;
	std::vector<VkDescriptorSet> cloudDescriptorSet;

};
