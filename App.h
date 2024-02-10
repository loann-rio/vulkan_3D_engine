#pragma once

#include "Window.h"
#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Swap_chain.h"

#include <string>
#include <memory>
#include <vector>
#include <iostream>

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
	void sierpinski(std::vector<Model::Vertex>& vertices, int depth, glm::vec2 left, glm::vec2 right, glm::vec2 top);
	void loadGameObjects();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffer();
	void freeCommandBuffer();
	void drawFrame();
	void recreateSwapChain();
	void recordCommandBuffer(int imageIndex);
	void renderGameObjects(VkCommandBuffer commandBuffer);

	Window window{ WIDTH, HEIGHT, "hello" };
	Device device{ window };

	std::unique_ptr<Swap_chain> swapChain;
	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<GameObject> gameObjects;
};

