#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "Camera.h"
#include "Buffer.h"
#include "Frame_info.h"
#include "GlTFModel.h"
#include "preBuild.h"
#include "Texture.h"


// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// imgui
//#define ENABLE_IMGUI

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#endif // ENABLE_IMGUI

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

    loadGameObjects(); 
    createRenderSystems();

    frameTimeVector = std::vector<float>(300);
}

App::~App() { globalPool = nullptr;  }

void App::run()
{
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
    for (auto& kv : listSpotLights) {
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

                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }

        frame = (frame + 1) % 100; 
        
	}

    vkQueueWaitIdle(device.presentQueue());
}
 
void App::loadGameObjects() {

    objectManager.startLoadModel();

    std::shared_ptr<Model> plane = createPlane(device, 10, 10, { 0, 0, 0 });
    auto plane1 = GameObject::createGameObject(device);
    plane1.setModel(plane);
    plane1.transform.translation.y = 0.1f;
    plane1.createDescriptorSet(*globalPool);
    objectManager.pushSyncGameObject(std::move(plane1));

    std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
    auto plane2 = GameObject::createGameObject(device);
    plane2.setModel(planeModel);
    plane2.transform.rotation.z = -pi<float> / 2;
    plane2.transform.translation.x = 10.f;
    plane2.transform.translation.y = 1.f;
    plane2.createDescriptorSet(*globalPool);
    objectManager.pushSyncGameObject(std::move(plane2));

    auto plane3 = GameObject::createGameObject(device);
    plane3.setModel(planeModel);
    plane3.transform.rotation.x = pi<float> / 2;
    plane3.transform.translation.z = 10.f;
    plane3.transform.translation.y = 1.f;
    plane3.createDescriptorSet(*globalPool);
    objectManager.pushSyncGameObject(std::move(plane3));

    /*std::shared_ptr<Model> icoSphere = PrebuiltModel::createIcoSphere(device, 1, 1);
    auto plane1 = GameObject::createGameObject(device);
    plane1.setModel(icoSphere);
    plane1.createDescriptorSet(*globalPool);
    objectManager.pushSyncGameObject(std::move(plane1));*/

    auto spotLight1 = GameObject::makeCamera(device, glm::radians(50.f), 1.f);
    spotLight1.transform.translation = { -4.0f, -1.0f, 5.5f };
    spotLight1.transform.rotation.y = pi<float> *2 / 5;
    spotLight1.transform.color = { 1.0, 1.0, 1.0, .7 };

    auto spotLight2 = GameObject::makeCamera(device, glm::radians(50.f), 1.f);
    spotLight2.transform.translation = { 1.0f, -2.0f, 2.5f };
    spotLight2.transform.rotation.y = pi<float> *2 / 5;
    spotLight2.transform.color = { 0.0, 1.0, 0.0, .7 };

    listSpotLights.emplace(spotLight1.getId(), std::move(spotLight1));
    listSpotLights.emplace(spotLight2.getId(), std::move(spotLight2));
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
