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


#include "renderSystem/GlobalRenderer.h"



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
	ObjectManager objectManager{ device, assetManager };

	GlobalRenderer renderer{ device, window, assetManager, objectManager };


	std::unique_ptr<DescriptorPool> globalPool{};

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;
	std::vector<std::unique_ptr<Buffer>> cloudBuffers;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;
	std::vector<VkDescriptorSet> cloudDescriptorSet;

};
