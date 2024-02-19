#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"
#include "preBuild.h"


// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


// std
#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>
#include <chrono>


struct GlobalUbo {
    glm::mat4 projectionView{};
    //alignas(16) glm::vec3 lightDir = glm::normalize(glm::vec3(1.f, -3.f, -1.f));
    glm::vec4 ambientLightColor{ 1.f, 1.f,  1.f, .02f };
    glm::vec3 lightPosition{ 5.f, -10.0f, 3.f };
    alignas(16) glm::vec4 lightColor{ 1.f, 1.f, .5f, 50.f }; // with w to be the intencity

};

App::App() { 
    globalPool = DescriptorPool::Builder(device)
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT)
        .build();

    loadGameObjects(); 
}

App::~App() { globalPool = nullptr;  }

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

    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSet(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]);
    }

	RenderSystem renderSystem{ device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };

    // camera setting
    Camera camera{};
    auto viewerObject = GameObject::createGameObject();
    viewerObject.transform.translation = { 2.0f, -1.0f, 2.5f };
    viewerObject.transform.rotation.y = 180;

    // user inputs
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    int frame = 0;

	while (!window.shouldClose())
	{
		glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspec = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);

		if (auto commandBuffer = renderer.beginFrame()) {
            int frameIndex = renderer.getFrameIndex();
            Frame_info::FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSet[frameIndex],
                gameObjects
            };
            
            // update
            GlobalUbo ubo{};
            ubo.projectionView = camera.getProjection() * camera.getView();
            //ubo.lightDir = glm::normalize(glm::vec3(cos(glm::radians((float)frame/100.0f)), -3.f, sin(glm::radians((float)frame/100.f))));

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render
			renderer.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(frameInfo);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
		}

        frame = (frame + 1) % 36000;
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    
    std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/smooth_vase.obj");

    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = { .0f, 0.0f, .0f };
    gameObject.transform.scale = { 1.f, 1.f , 1.f };
    gameObjects.emplace(gameObject.getId(), std::move(gameObject));


    Model::Builder modelBuilder{};
    modelBuilder.vertices = {
            {{-0.5f, 0, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, 0,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, 0, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, 0,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}} };
    modelBuilder.indices = { 0, 1, 3, 0, 2, 3 };

    auto plane = GameObject::createGameObject();
    plane.model = std::make_unique<Model>(device, modelBuilder);
    plane.transform.translation = { .0f, .0f, .0f };
    plane.transform.scale = { 20.f, 1.f , 20.f };
    gameObjects.emplace(plane.getId(), std::move(plane));

}
