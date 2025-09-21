#pragma once


#include "base/Window.h"
#include "base/device.h"
#include "objects/GameObject.h"

#include "base/descriptors.h"

#include "render/Renderer.h"
#include "render/GlobalRenderSystem.h"
#include "render/TextOverlay.h"
#include "objects/ObjectManager.h"



#include <memory>
#include <vector>
#include <deque>


class App
{
public:
	static constexpr int WIDTH = 1600;
	static constexpr int HEIGHT = 1200;

	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	void run();

private:
	
	void getFrameRate(float lastFrameTime);
	void createRenderSystems();

	Window window{ WIDTH, HEIGHT, "vulkan engine" };
	Device device{ window };
	Renderer renderer{ window, device };

	std::unique_ptr<DescriptorPool> globalPool{};

	std::vector<float> frameTimeVector;
	float frameTimeSum = 0;

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;
	std::vector<std::unique_ptr<Buffer>> terrainBuffers;

	// render systems
	std::shared_ptr<GlobalRenderSystem> gltfRenderSystem;
	std::shared_ptr<GlobalRenderSystem> objRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystem;
	std::shared_ptr<GlobalRenderSystem> depthRenderSystemGltf;
	std::shared_ptr<GlobalRenderSystem> terrainRenderSystem;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;

	// object management:
	ObjectManager objectManager{ device };
};

