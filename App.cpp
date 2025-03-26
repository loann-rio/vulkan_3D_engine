#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"
#include "GlTFModel.h"
#include "Texture.h"


// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// imgui
#define ENABLE_IMGUI
#include "BasicUI.h"

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
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 64)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 64)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT*64)
        .build();

    objectManager.startLoadModel(*globalPool); 
    createRenderSystems();

    frameTimeVector = std::vector<float>(300);
}

App::~App() {
    globalPool = nullptr; 
}

void App::run()
{
    // ui
    BasicUI imgui{ device, window.getGLFWwindow(), renderer.getSwapChainRenderPass() };

    TextOverlay textOverlay(device, renderer.getSwapChainRenderPass());
    textOverlay.prepareResources(*globalPool);

    // camera setting
    auto cameraObject = GameObject::makeCamera(device, glm::radians(50.f), renderer.getAspectRatio());
    cameraObject.transform.translation = { 2.0f, -1.0f, 2.5f }; 
    cameraObject.transform.rotation.y = pi<float> * 1/3; 

    // user inputs
    KeyboardMovementController cameraController{};

    // UBO
    GlobalUbo ubo{};
    SpotLightUbo spotLightUbo{};
    

    int i = 0;
    for (auto& kv : *objectManager.getSpotLights()) {
        auto& spot = kv.second;
        spotLightUbo.spotLight[i++] = spot.getSpotLightInfo(true);
        if (i > DepthSwapChain::MAX_DEPTH_RENDER_COUNT) break;
    }

    spotLightUbo.numLights = i;
    
    shadowUboBuffer[0]->writeToBuffer(&spotLightUbo);
    shadowUboBuffer[0]->flush();

    shadowUboBuffer[1]->writeToBuffer(&spotLightUbo);
    shadowUboBuffer[1]->flush();

    // start timer
    auto currentTime = std::chrono::high_resolution_clock::now();

    int frame = 0;
	while (!window.shouldClose())
	{
		glfwPollEvents();

        // add loaded async model to gameObjectmap
        objectManager.pushModel(*globalPool);

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
        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, cameraObject);

        /////// start frame ///////
        if (!renderer.aquireNextImage()) continue;
        
        int frameIndex = renderer.getFrameIndex();


        FrameInfo frameInfo{  
            frameIndex,
            frameTime,
            spotLightUbo.numLights,
            cameraObject.transform.translation, 
            objectManager.getGameObject()
        };  

        std::vector<VkDescriptorSet> descriptorSets{ globalDescriptorSet[frameIndex], shadowDescriptorSet[frameIndex] };

        /////// update objects ///////

        ubo.projection = cameraObject.camera->getProjection(); 
        ubo.view = cameraObject.camera->getView(); 
        ubo.inverseView = cameraObject.camera->getInverseView(); 
        
        uboBuffers[frameIndex]->writeToBuffer(&ubo);
        uboBuffers[frameIndex]->flush();       

        /////// render depthframe ///////
        {
            std::lock_guard<std::mutex> lock(device.getGraphicMutex());

            vkQueueWaitIdle(device.presentQueue());
            if (frame == 0) {
                renderer.renderDepthImage(frameInfo, { depthRenderSystemGltf, depthRenderSystem }, descriptorSets);
            }

            if (auto commandBuffer = renderer.beginFrame()) {

                // render
                renderer.beginSwapChainRenderPass(commandBuffer);

                gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets); 
                objRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets);

                //pointLightSystem->render(commandBuffer, frameInfo, { globalDescriptorSet[frameIndex] } );
                textOverlay.renderText(commandBuffer, frameInfo); 

                imgui.drawUI(commandBuffer, &(objectManager.getGameObject()->begin())->second);

                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }

        frame = (frame + 1) % 10;

	}

    vkQueueWaitIdle(device.presentQueue());
}
 

void App::createRenderSystems()
{

    /// global buffer
    uboBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
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

    globalDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]); 
    } 

    //// shadow buffer
    shadowUboBuffer.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT); 
    for (int i = 0; i < shadowUboBuffer.size(); i++)
    {
        shadowUboBuffer[i] = std::make_unique<Buffer>( 
            device, 
            sizeof(SpotLightUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            device.properties.limits.minUniformBufferOffsetAlignment 
        ); 

        shadowUboBuffer[i]->map(); 
    } 
     
    auto shadowSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) 
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, DepthSwapChain::MAX_DEPTH_RENDER_COUNT)
        .build();

    shadowDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < shadowDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = shadowUboBuffer[i]->descriptorInfo();
        auto depthInfo = renderer.getShadowImageInfo(i);

        DescriptorWriter(*shadowSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, depthInfo, DepthSwapChain::MAX_DEPTH_RENDER_COUNT)
            .build(shadowDescriptorSet[i]); 
    }


    /// render systems

    gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(
        device, renderer.getSwapChainRenderPass(), { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() },
        "GlTFshader.vert.spv", "GlTFshader.frag.spv");

    objRenderSystem = GlobalRenderSystem::create<Model>(
        device, renderer.getSwapChainRenderPass(), { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() },
        "simple_shader.vert.spv", "simple_shader.frag.spv");

    depthRenderSystem = GlobalRenderSystem::create<Model>(
        device, renderer.getDepthRenderPass(), { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() },
        "shadowmap.vert.spv");

    depthRenderSystemGltf = GlobalRenderSystem::create<GlTFModel::ModelGltf>(
        device, renderer.getDepthRenderPass(), { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() },
        "shadowmap.vert.spv");

    pointLightSystem = std::make_unique<PointLightSystem>(device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
    
}

void App::getFrameRate(float lastFrameTime)
{
    float v = 1 / (lastFrameTime * 100);
    frameTimeSum += v;
    frameTimeSum -= frameTimeVector[0];

    frameTimeVector.push_back(v);
    frameTimeVector.erase(frameTimeVector.begin()); 
}
