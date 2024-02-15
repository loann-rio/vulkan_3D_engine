#include "App.h"

#include "KeyboardMovementController.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>
#include <chrono>

struct GlobalUbo {
    glm::mat4 projectionView{};
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.f, -3.f, -1.f));
};

App::App() { loadGameObjects(); }

App::~App() { }

void App::run()
{
    std::vector<std::unique_ptr<Buffer>> uboBuffers(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++)
    {
        uboBuffers[i] = std::make_unique<Buffer>(
            device,
            sizeof(GlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            device.properties.limits.minUniformBufferOffsetAlignment
        );

        uboBuffers[i]->map();
    }

	RenderSystem renderSystem{ device, renderer.getSwapChainRenderPass() };
    Camera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = GameObject::createGameObject();
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

	while (!window.shouldClose())
	{
		glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspec = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 10.0f);

		if (auto commandBuffer = renderer.beginFrame()) {
            int frameIndex = renderer.getFrameIndex();
            Frame_info::FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera
            };
            
            // update
            GlobalUbo ubo{};
            ubo.projectionView = camera.getProjection() * camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render

			renderer.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(frameInfo, gameObjects);
			renderer.endSwapChainRenderPass(commandBuffer);
			renderer.endFrame();
		}
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/smooth_vase.obj");

    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = { .0f, .5f, 2.5f };
    gameObject.transform.scale = { 3.f, 3.f , 3.f };
    gameObjects.push_back(std::move(gameObject));
}
