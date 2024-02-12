#include "App.h"

#include "RenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>


App::App() { loadGameObjects(); }

App::~App() { }

void App::run()
{
	RenderSystem renderSystem{ device, renderer.getSwapChainRenderPass() };
	while (!window.shouldClose())
	{
		glfwPollEvents();
		if (auto commandBuffer = renderer.beginFrame()) {
			renderer.beginSwapChainRenderPass(commandBuffer);
			renderSystem.renderGameObjects(commandBuffer, gameObjects);
			renderer.endSwapChainRenderPass(commandBuffer);
			renderer.endFrame();

		}
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
	std::vector<Model::Vertex> vertices{
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} 
	};
	
	for (unsigned int i = 0; i < 1000; i++)
	{
		auto model = std::make_shared<Model>(device, vertices);
		auto triangle = GameObject::createGameObject();
		triangle.model = model;
		triangle.color = { static_cast <float> (rand()) / static_cast <float> (RAND_MAX) ,static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX)};
		//triangle.transform2d.translation.x = .8f;
		triangle.transform2d.scale = { .5f, .5f };
		triangle.transform2d.rotation = .25f * 2 * pi<float>;

		gameObjects.push_back(std::move(triangle));
	}
	
}
