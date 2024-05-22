#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"
#include "preBuild.h"
#include "point_light_system.h"
#include "preBuild.h"
#include "Texture.h"


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
#include <string>

App::App() { 
    globalPool = DescriptorPool::Builder(device)
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 12)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT*8)
        .build();

    loadGameObjects(); 

    frameTimeVector = std::vector<float>(300);
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

    auto globalSetLayout = DescriptorSetLayout::Builder(device).addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).build();

    std::vector<VkDescriptorSet> globalDescriptorSet(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]);
    }

    for (auto& kv : gameObjects)
    {
        auto& obj = kv.second;
        if (obj.model == nullptr) continue;
        obj.createDescriptorSet(*globalPool, device);
    }

    PointLightSystem pointLightSystem{ device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
	RenderSystem renderSystem{ device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };

    TextOverlay textOverlay{ device, renderer.getSwapChainRenderPass() };
    textOverlay.prepareResources(*globalPool);


    // camera setting
    Camera camera{};
    auto viewerObject = GameObject::createGameObject(device);
    viewerObject.transform.translation = { 2.0f, -1.0f, 2.5f };
    viewerObject.transform.rotation.y = 180;

    float aspec = renderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);

    // user inputs
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    int frame = 0;
	while (!window.shouldClose())
	{
		glfwPollEvents();


        // calculate frame time
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // show fps count on screen
        getFrameRate(frameTime);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << frameTimeSum << " fps";

        textOverlay.beginTextUpdate();
        textOverlay.addText(ss.str(), 10, 10, TextOverlay::alignLeft, renderer.getWidth(), renderer.getHeight());
        textOverlay.endTextUpdate();

        // move camera on event 
        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        

		if (auto commandBuffer = renderer.beginFrame()) {
            int frameIndex = renderer.getFrameIndex();

            FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSet,
                gameObjects
            };

            
            // update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();

            pointLightSystem.update(frameInfo, ubo, frame);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render
			renderer.beginSwapChainRenderPass(commandBuffer);

            renderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            textOverlay.renderText(frameInfo);

            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
		}

        frame = (frame + 1) % 36000; 
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    
    std::shared_ptr<Model> model_city = Model::createModelFromFile(device, "models/viking_room.obj", "textures/viking_room.png");
    auto Lowpoly_City = GameObject::createGameObject(device);
    Lowpoly_City.transform.rotation.x = pi<float> / 2;
    Lowpoly_City.model = model_city;
    gameObjects.emplace(Lowpoly_City.getId(), std::move(Lowpoly_City));


    std::shared_ptr<Model> model_city1 = Model::createModelFromFile(device, "models/viking_room.obj", "textures/Palette.jpg");
    auto Lowpoly_City1= GameObject::createGameObject(device);
    Lowpoly_City1.transform.rotation.x = pi<float> / 2;
    Lowpoly_City1.model = model_city1;
    Lowpoly_City1.transform.translation.z = 2;
    gameObjects.emplace(Lowpoly_City1.getId(), std::move(Lowpoly_City1));

}

void App::getFrameRate(float lastFrameTime)
{
    float v = 1 / (lastFrameTime * 300);
    frameTimeSum += v;
    frameTimeSum -= frameTimeVector[0];

    frameTimeVector.push_back(v);
    frameTimeVector.erase(frameTimeVector.begin());
}
