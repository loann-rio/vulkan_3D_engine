#pragma once

#include "Window.h"
#include "device.h"
#include "GameObject.h"

#include "descriptors.h"

#include "Renderer.h"
#include "GlobalRenderSystem.h"
#include "point_light_system.h"
#include "TextOverlay.h"

#include <memory>
#include <vector>
#include <deque>

template<class T>
constexpr T pi = T(3.1415926535897932385L);

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
	void loadGameObjects();
	void createRenderSystems();

	Window window{ WIDTH, HEIGHT, "vulkan engine" };
	Device device{ window };
	Renderer renderer{ window, device };

	std::unique_ptr<DescriptorPool> globalPool{};
	GameObject::Map gameObjects;

	std::vector<float> frameTimeVector;
	float frameTimeSum = 0;

	// buffers
	std::vector<std::unique_ptr<Buffer>> uboBuffers;
	std::vector<std::unique_ptr<Buffer>> shadowUboBuffer;

	// render systems
	std::shared_ptr<GlobalRenderSystem> gltfRenderSystem;
	std::shared_ptr<GlobalRenderSystem> objRenderSystem;
	std::shared_ptr<GlobalRenderSystem> DepthRenderSystem;

	std::unique_ptr<PointLightSystem> pointLightSystem;

	//std::unique_ptr<TextOverlay> textOverlay;

	// global descriptor sets
	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
};

