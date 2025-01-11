#include "App.h"

// local
#include "KeyboardMovementController.h"

#include "GLTFrenderSystem.h"
#include "RenderSystem.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"

#include "point_light_system.h"
#include "preBuild.h"
#include "GenerateTerrainTile.h"
#include "ChunckManager.h"

#include "Texture.h"
#include "TextOverlay.h"

#include "TerrainRenderer.h"

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
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 40)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 40)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT*40)
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
        if (obj.model != nullptr) {
            obj.model->createDescriptorSet(*globalPool, device);
        }
        else if (obj.gltfModel != nullptr) {
            obj.gltfModel->createDescriptorSet(*globalPool, device);
        }
        
    }

    PointLightSystem pointLightSystem{  device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
	RenderSystem renderSystem{          device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
    GlTFrenderSystem GlTfrenderSystem{  device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };
    TerrainRenderer terrainRenderer{    device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() };

    TextOverlay textOverlay{ device, renderer.getSwapChainRenderPass() };
    textOverlay.prepareResources(*globalPool);


    // camera setting
    Camera camera{};
    auto viewerObject = GameObject::createGameObject(device);
    viewerObject.transform.translation = { 2.0f, -1.0f, 2.5f };
    viewerObject.transform.rotation.y = pi<float> * 1/3;

    float aspec = renderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspec, .1f, 100.0f);

    // user inputs
    KeyboardMovementController cameraController{};


    // chunkj management
    ChunckManager chunks{ device };

    // update terrain:
    chunks.updateChuncks(
        viewerObject.transform.translation.x / chunks.sizePlaneX,
        viewerObject.transform.translation.z / chunks.sizePlaneZ);



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

            
            // update ubo
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();
            
            // update objects
            pointLightSystem.update(frameInfo, ubo, frame);


            // render
			renderer.beginSwapChainRenderPass(commandBuffer);
            
            terrainRenderer.renderTerrain(frameInfo, chunks.chunks);
            terrainRenderer.renderTerrain(frameInfo, gameObjects);
            //renderSystem.renderGameObjects(frameInfo);
            //GlTfrenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            textOverlay.renderText(frameInfo);

            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
		}

        //frame = (frame + 1) % 36000; 
	}

	vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    
    /*for (int i = 0; i < 10; i++)
    {
        GenerateTerrainTile tileGenerator;
        std::shared_ptr<Model> terrain = tileGenerator.generateMesh(device);
        tileGenerator.generateHeightMap(device);
        terrain->texture = tileGenerator.noiseTexture;

        auto terrain_object = GameObject::createGameObject(device);
        terrain_object.model = terrain;
        gameObjects.emplace(terrain_object.getId(), std::move(terrain_object));
    }*/
    for (int i = 0; i < 5; i ++)
    {
        GenerateTerrainTile tileGenerator;

        tileGenerator.x = 0;
        tileGenerator.z = 0;

        tileGenerator.detailX = 100;
        tileGenerator.detailZ = 100;

        tileGenerator.offsetX = ((float) i) * tileGenerator.detailX;
        tileGenerator.offsetZ = (float)0 * tileGenerator.detailX;

        tileGenerator.scale = 25.f * 2;

        std::shared_ptr<Model> terrain = tileGenerator.generateMesh(device);

        tileGenerator.generateHeightMap(device);
        terrain->texture = tileGenerator.noiseTexture;

        auto terrain_object = GameObject::createGameObject(device);
        terrain_object.transform.translation = { 5.f * i, 0, 6 };
        terrain_object.model = terrain;
        gameObjects.emplace(terrain_object.getId(), std::move(terrain_object));
    }
    
   /* {
        GenerateTerrainTile tileGenerator;

        tileGenerator.x = 1;
        tileGenerator.z = 0;

        tileGenerator.detailX = 30;
        tileGenerator.detailZ = 30;

        tileGenerator.offsetX = (float)1 * (float)tileGenerator.detailX / tileGenerator.scale;
        tileGenerator.offsetZ = (float)0 * (float)tileGenerator.detailZ / tileGenerator.scale;

        tileGenerator.scale = 25.f / 2;

        std::shared_ptr<Model> terrain = tileGenerator.generateMesh(device);

        tileGenerator.generateHeightMap(device);
        terrain->texture = tileGenerator.noiseTexture;

        auto terrain_object = GameObject::createGameObject(device);
        terrain_object.transform.translation = { 5, 0, 0 };
        terrain_object.model = terrain;
        gameObjects.emplace(terrain_object.getId(), std::move(terrain_object));
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
