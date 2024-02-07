#pragma once

#include "Window.h"
#include "Pipeline.h"
#include "device.h"
#include "Swap_chain.h"
#include "Model.h"

#include <string>
#include <memory>
#include <vector>

class App
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	void run();

private:
	void loadModels();
	void createPipelineLayout();
	void createPipeline();
	void createCommandBuffer();
	void drawFrame();

	Window window{ WIDTH, HEIGHT, "hello" };

	Device device{ window };

	Swap_chain swapChain{ device, window.getExtent() };
	
	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;
	std::vector<VkCommandBuffer> commandBuffers;
	std::unique_ptr<Model> model;
};

