#include "App.h"

// local
#include "KeyboardMovementController.h"

#include "GlobalRenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"

#include "GlTFModel.h"
#include "point_light_system.h"
#include "preBuild.h"

#include "Texture.h"
#include "TextOverlay.h"


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
#include <sstream>
#include <iomanip>



App::App() { 
    globalPool = DescriptorPool::Builder(device)
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 16)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 10)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT*16)
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

    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSet(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        auto shadowInfo = renderer.getShadowImageInfo(i);

        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &shadowInfo)
            .build(globalDescriptorSet[i]);
    }

    /*auto shadowSetLayout = DescriptorSetLayout::Builder(device).addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS).build();

    std::vector<VkDescriptorSet> shadowDescriptorSet(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < shadowDescriptorSet.size() && i < 2; i++)
    {
        auto shadowInfo = renderer.getShadowImageInfo(i);

        DescriptorWriter(*shadowSetLayout, *globalPool)
            .writeImage(0, &shadowInfo)
            .build(shadowDescriptorSet[i]); 
    }*/

    for (auto& kv : gameObjects) {
        auto& obj = kv.second;
        if (obj.hasModel) {
            obj.createDescriptorSet(*globalPool);
        }
    }

    PointLightSystem pointLightSystem{  device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
 

    GlobalRenderSystem gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(
        device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(), 
        "GlTFshader.vert.spv", "GlTFshader.frag.spv" );

    GlobalRenderSystem objRenderSystem = GlobalRenderSystem::create<Model>(
        device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(),
        "simple_shader.vert.spv", "simple_shader.frag.spv");

    GlobalRenderSystem DepthRenderSystem = GlobalRenderSystem::createDepth<Model>(
        device, renderer.getDepthRenderPass(), globalSetLayout->getDescriptorSetLayout(),
        "shadowmap.vert.spv", "");

    GlobalRenderSystem depthVisualisation = GlobalRenderSystem::create<Model>(
        device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(),
        "depthView.vert.spv", "depthView.frag.spv");
    depthVisualisation.setType(QUAD_MODEL);

    TextOverlay textOverlay{ device, renderer.getSwapChainRenderPass() };
    textOverlay.prepareResources(*globalPool);

    // camera setting
    Camera camera{};
    auto viewerObject = GameObject::createGameObject(device);
    viewerObject.transform.translation = { 2.0f, -1.0f, 2.5f };
    viewerObject.transform.rotation.y = pi<float> * 1/3;

    float aspec = renderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);


    Camera lightSource{};
    auto lightSourceObject = GameObject::createGameObject(device);
    lightSourceObject.transform.translation = { 2.0f, -1.0f, 2.5f };
    lightSourceObject.transform.rotation.y = pi<float> *1 / 3;

    lightSource.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);
    lightSource.setViewYXZ(lightSourceObject.transform.translation, lightSourceObject.transform.rotation); 

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

        {
            // show fps count on screen
            getFrameRate(frameTime);

            std::stringstream ss("");
            ss << std::fixed << std::setprecision(2) << frameTimeSum << " fps";

            textOverlay.beginTextUpdate();
            textOverlay.addText(ss.str(), 10, 10, TextOverlay::alignLeft, renderer.getWidth(), renderer.getHeight());
            textOverlay.endTextUpdate();
        }
        
        // move camera on event 
        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        if (!renderer.aquireNextImage()) continue;

        GlobalUbo ubo{};

        // render depthframe
        if (auto depthCommandBuffer = renderer.beginDepthFrame()) {
            int depthFrameIndex = renderer.getDepthIndex();

            FrameInfo frameinfo{
                depthFrameIndex,
                frameTime,
                depthCommandBuffer,
                lightSource,
                globalDescriptorSet,
                gameObjects
            };


            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            ubo.lightProjection = lightSource.getProjection();
            ubo.lightView = lightSource.getView();

            uboBuffers[depthFrameIndex]->writeToBuffer(&ubo);
            uboBuffers[depthFrameIndex]->flush();
             
            
            // render
            renderer.beginShadowRenderPass(depthCommandBuffer);

            DepthRenderSystem.renderGameObjects(frameinfo);

            renderer.endShadowRenderPass(depthCommandBuffer);
            renderer.endDepthFrame();
        }
        
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
            
            
            // update objects
            pointLightSystem.update(frameInfo, ubo, frame);


            // render
			renderer.beginSwapChainRenderPass(commandBuffer);

            gltfRenderSystem.renderGameObjects(frameInfo);
            objRenderSystem.renderGameObjects(frameInfo);

            pointLightSystem.render(frameInfo);
            textOverlay.renderText(frameInfo);

            depthVisualisation.renderGameObjects(frameInfo);

            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();

		}

        renderer.submitCommandBuffers();

        frame = (frame + 1) % 3; 
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    
    std::shared_ptr<Model> viking_room = Model::createModelFromFile(device, "model/viking_room.obj", "textures/viking_room.png");
    auto Lowpoly_City = GameObject::createGameObject(device);
    Lowpoly_City.transform.rotation = { pi<float> / 2, pi<float>, 0 };
    Lowpoly_City.transform.translation = { 7, 0, 7 };
    Lowpoly_City.setModel(viking_room);
    gameObjects.emplace(Lowpoly_City.getId(), std::move(Lowpoly_City));

   /* std::shared_ptr<GlTFModel::ModelGltf> damagedHelmet = GlTFModel::createModelFromFile(device, "model/2.0/damagedhelmet/gltf/damagedhelmet.gltf");
    auto godh = GameObject::createGameObject(device);
    godh.transform.rotation = { pi<float> / 2, pi<float>, 0 };
    godh.transform.translation = { 7, 1, 5 };
    godh.transform.scale = { 0.5f, 0.5f, 0.5f };
    godh.setModel(damagedHelmet);
    gameObjects.emplace(godh.getId(), std::move(godh));*/

    std::shared_ptr<GlTFModel::ModelGltf> spotlight = GlTFModel::createModelFromFile(device, "model/spotlight/scene.gltf");
    auto godh = GameObject::createGameObject(device);
    godh.transform.rotation = { pi<float> / 2, pi<float>, 0 };
    godh.transform.translation = { 7, 1, 5 };
    godh.transform.scale = { 0.5f, 0.5f, 0.5f };
    godh.setModel(spotlight);
    gameObjects.emplace(godh.getId(), std::move(godh));

    std::shared_ptr<Model> plane = createPlane(device, 10, 10, {0, 0, 0});
    auto plane1 = GameObject::createGameObject(device);
    plane1.setModel(plane);
    plane1.transform.translation.y = 0.1f;
    gameObjects.emplace(plane1.getId(), std::move(plane1));

    std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
    auto plane2 = GameObject::createGameObject(device);
    plane2.setModel(planeModel);
    plane2.transform.rotation.z = -pi<float> / 2;
    plane2.transform.translation.x = 10.f;
    plane2.transform.translation.y = 1.f;
    gameObjects.emplace(plane2.getId(), std::move(plane2));

    auto plane3 = GameObject::createGameObject(device);
    plane3.setModel(planeModel);
    plane3.transform.rotation.x = pi<float> / 2;
    plane3.transform.translation.z = 10.f;
    plane3.transform.translation.y = 1.f;
    gameObjects.emplace(plane3.getId(), std::move(plane3));

    Model::Builder modelBuilder{};
    modelBuilder.vertices = {
        {{-1.0f,  1.0f, 0.f}, {0, 0, 0}, {0, 0, 0}, { 0.0f, 1.0f }}, // Top-left
        {{-0.6f,  1.0f, 0.f}, {0, 0, 0}, {0, 0, 0}, {1.0f, 1.0f}}, // Top-right
        {{-1.0f,  0.6f, 0.f}, {0, 0, 0}, {0, 0, 0}, {0.0f, 0.0f}}, // Bottom-left
        {{-0.6f,  0.6f, 0.f}, {0, 0, 0}, {0, 0, 0}, {1.0f, 0.0f}}  // Bottom-right
    };

    modelBuilder.indices = {
        0, 1, 2, // First triangle
        2, 1, 3  // Second triangle
    };

    std::shared_ptr<Model> quad = std::make_unique<Model>(device, modelBuilder, "textures/emptyTexture.jpg");
    auto depthView = GameObject::createGameObject(device);
    depthView.setModel(quad);
    depthView.setModelType(QUAD_MODEL);
    gameObjects.emplace(depthView.getId(), std::move(depthView));
    

    /*std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f},
      {.1f, 1.f, .1f},
      {1.f, 1.f, .1f},
      {.1f, 1.f, 1.f},
      {.1f, .1f, 1.f},
      {1.f, 1.f, 1.f}
    };

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = GameObject::makePointLight(device, 1.f, 0.05f, lightColors[i]);
        auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), { 0.f, -1.0f, 0.f });
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 4.f)) + glm::vec3{ 7, 0, 7 };
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }*/

}

void App::getFrameRate(float lastFrameTime)
{
    float v = 1 / (lastFrameTime * 100);
    frameTimeSum += v;
    frameTimeSum -= frameTimeVector[0];

    frameTimeVector.push_back(v);
    frameTimeVector.erase(frameTimeVector.begin());
}
