#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"
#include "preBuild.h"
#include "point_light_system.h"
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
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT)
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
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    Texture texture{device};
    texture.createTextureImage("textures/viking_room.png");
    texture.createTextureImageView();
    texture.createTextureSampler();



    std::vector<VkDescriptorSet> globalDescriptorSet(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        VkDescriptorImageInfo imageInfo = texture.getImageInfo();

        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSet[i]);
    }

    PointLightSystem pointLightSystem{ device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
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
        std::cout << (1 / frameTime) << "\n";
        currentTime = std::chrono::high_resolution_clock::now();
        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspec = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);

		if (auto commandBuffer = renderer.beginFrame()) {
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                camera,
                globalDescriptorSet[frameIndex],
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

            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
		}

        frame = (frame + 1) % 36000;
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    
    //std::shared_ptr<Model> model_city = Model::createModelFromFile(device, "models/Lowpoly_City_Free_Pack.obj");

    //auto Lowpoly_City = GameObject::createGameObject();
    //Lowpoly_City.model = model_city;
    //Lowpoly_City.transform.translation = { .0f, 0.0f, .0f };
    //Lowpoly_City.transform.scale = { .01f, -0.01f , .01f };
    ////gameObject.transform.rotation.x = glm::radians(90.0f);
    //gameObjects.emplace(Lowpoly_City.getId(), std::move(Lowpoly_City));

    std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/viking_room.obj");

    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = { .0f, 0.0f, .0f };
    gameObject.transform.scale = { 1.f, 1.f , 1.f };
    gameObject.transform.rotation.x = glm::radians(90.0f);
    gameObjects.emplace(gameObject.getId(), std::move(gameObject));


   /* Model::Builder modelBuilder{};
    modelBuilder.vertices = {
            {{-0.5f, 0, -0.5f}, {.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}},
            {{-0.5f, 0,  0.5f}, {.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, 0, -0.5f}, {.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}},
            {{ 0.5f, 0,  0.5f}, {.5f, .5f, .5f}, {0.0f, -1.0f, 0.0f}} };
    modelBuilder.indices = { 0, 1, 3, 0, 2, 3 };*/

    /*auto plane = GameObject::createGameObject();
    plane.model = std::make_unique<Model>(device, modelBuilder);
    plane.transform.translation = { .0f, .0f, .0f };
    plane.transform.scale = { 2.f, 1.f , 2.f };
    gameObjects.emplace(plane.getId(), std::move(plane));*/

    /*std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f},
      
      {.1f, 1.f, .1f},

      {1.f, 1.f, .1f},
      {.1f, 1.f, 1.f},
                  {.1f, .1f, 1.f},

      {1.f, 1.f, 1.f}  
    };

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = GameObject::makePointLight(0.5f, 0.05f, lightColors[i]);
        auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.f, -1.0f, 0.f});
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }*/

    auto pointLight = GameObject::makePointLight(.7f, 0.0f, {1.0f, 0.5f, 0.f});
    pointLight.transform.translation = {-0.27f, -0.7f, .6f};
    gameObjects.emplace(pointLight.getId(), std::move(pointLight));

    auto pointLight1 = GameObject::makePointLight(0.7f, 0.0f, { 1.0f, 0.5f, 0.f });
    pointLight1.transform.translation = { -0.27f, -0.7f, -0.7f };
    gameObjects.emplace(pointLight1.getId(), std::move(pointLight1));

    auto pointLight2 = GameObject::makePointLight(0.2f, 0.05f, { 1.0f, 0.f, 0.f });
    pointLight2.transform.translation = { 0.1f, -0.1f, .0f };
    gameObjects.emplace(pointLight2.getId(), std::move(pointLight2));
}
