#include "App.h"

// local

#include "base/Buffer.h"
#include "base/Frame_info.h"
#include "objects/Texture.h"
#include "objects/KeyboardMovementController.h"
#include "render/Camera.h"
#include "model/GlTFModel.h"


// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// imgui
#include "objects/BasicUI.h"

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
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 320)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .build();

    objectManager.startLoadModel(*globalPool); 
    createRenderSystems();

    frameTimeVector = std::vector<float>(50);
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

    // user inputs
    KeyboardMovementController cameraController{};

    // UBO
    GlobalUbo ubo{};
    SpotLightUbo spotLightUbo{};

    //std::unique_ptr<Texture> cubeMap = Texture::createCubeMap(device, "skybox\\cubemap_space.ktx");
    
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
        
        //// move camera on event ////
        {
            if (!imgui.isWindowSelected)
                cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, objectManager.get(objectManager.mainCamera));
            dynamic_cast<GameObjectCamera*>(objectManager.get(objectManager.mainCamera))->updateCameraView();
        }

        /////// start frame ///////
        if (!renderer.aquireNextImage()) continue;
        
        int frameIndex = renderer.getFrameIndex();

        {
            // show fps count on screen
            getFrameRate(frameTime);

            std::stringstream ss("");
            ss << std::fixed << std::setprecision(2) << frameTimeSum << " fps";

            textOverlay.beginTextUpdate(frameIndex);
            textOverlay.addText(frameIndex, ss.str(), 10, 10, TextOverlay::alignLeft, renderer.getWidth(), renderer.getHeight()); 
            textOverlay.endTextUpdate(frameIndex); 
        }

        /////// update objects ///////

        // loop on games objects
        {
            std::shared_ptr<GameObject::Map> objects = objectManager.getGameObjects();
            for (auto obj = objects->begin(); obj != objects->end(); obj++) {
                obj->second->loop(&objectManager);
            }
        }

        // update GLTF game objects
        {
            std::vector<GameObjectModel*> objects = objectManager.getByType<GameObjectModel>();
            for (auto obj : objects) {
                obj->update(frameTime);
            }
        }

        // update camera
        {
            auto* camObj = dynamic_cast<GameObjectCamera*>(objectManager.get(objectManager.mainCamera));
            if (camObj->camera->aspectRatio != renderer.getAspectRatio()) {
                camObj->camera->setPerspectiveProjection(renderer.getAspectRatio());
            }

            ubo.projection = camObj->camera->getProjection();
            ubo.view = camObj->camera->getView(); 
            ubo.inverseView = camObj->camera->getInverseView();
            ubo.lightPos = camObj->transform.translation; 
        }

        // update pointLight 
        {
            std::vector<GameObjectPointLight*> pointLigths = objectManager.getByType<GameObjectPointLight>();
            uint16_t i = 0;
            for (auto lightObj : pointLigths) {
                if (lightObj->transform.color.w)
                    ubo.pointLights[i++] = PointLight{ glm::vec4(lightObj->transform.translation, lightObj->transform.scale.x), lightObj->transform.color };
                if (i >= MAX_LIGHT) break;
            }
            ubo.numLights = i;
        }

        uboBuffers[frameIndex]->writeToBuffer(&ubo); 
        uboBuffers[frameIndex]->flush(); 


        // update spotLight
        {
            uint16_t i = 0; 
            std::vector<GameObjectSpotLight*> spotLigths = objectManager.getByType<GameObjectSpotLight>();
            for (auto lightObj : spotLigths) {
                if (lightObj->transform.color.w != 0)
                    spotLightUbo.spotLight[i++] = lightObj->getSpotLightInfo(true);
                if (i >= DepthSwapChain::MAX_DEPTH_RENDER_COUNT) break;
            }
            spotLightUbo.numLights = i;

            shadowUboBuffer[frameIndex]->writeToBuffer(&spotLightUbo);
            shadowUboBuffer[frameIndex]->flush();
        }

        FrameInfo frameInfo{
            frameIndex,
            frameTime,
            spotLightUbo.numLights,
            objectManager.get(objectManager.mainCamera)->transform.translation,
            objectManager.getByType<GameObjectModel>()
        };

        /////// render frame ///////
        {
            std::vector<VkDescriptorSet> descriptorSets{ globalDescriptorSet[frameIndex], shadowDescriptorSet[renderer.getDepthIndex()]};

            std::lock_guard<std::mutex> lock(device.getGraphicMutex());

            vkQueueWaitIdle(device.presentQueue());
            if (frame == 0) {
                renderer.renderDepthImage(frameInfo, { depthRenderSystem, depthRenderSystemGltf }, descriptorSets);
            } 

            if (auto commandBuffer = renderer.beginFrame()) {
                 
                // render
                renderer.beginSwapChainRenderPass(commandBuffer); 
                 
                gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets); 
                objRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets); 
                terrainRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets);

                textOverlay.renderText(commandBuffer, frameInfo); 

                imgui.drawUI(commandBuffer, &objectManager);

                renderer.endSwapChainRenderPass(commandBuffer); 
                renderer.endFrame();
            } 
        }   

        frame = (frame + 1) % 1;
        
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

	RenderSystemBuilder gltfBuilder{};
	gltfBuilder.fragFilepath = "shaders\\GlTFshader.frag.spv";
	gltfBuilder.vertFilepath = "shaders\\GlTFshader.vert.spv";
    gltfBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
	gltfBuilder.renderPass = renderer.getSwapChainRenderPass();

    gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, gltfBuilder); 

    RenderSystemBuilder objBuilder{};
    objBuilder.fragFilepath = "shaders\\simple_shader.frag.spv";
    objBuilder.vertFilepath = "shaders\\simple_shader.vert.spv";
    objBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
    objBuilder.renderPass = renderer.getSwapChainRenderPass();
    objBuilder.hasMultipleInstance = true;

    objRenderSystem = GlobalRenderSystem::create<Model>(device, objBuilder); 

    RenderSystemBuilder objShadowBuilder{};
    objShadowBuilder.vertFilepath = "shaders\\shadowmap.vert.spv";
    objShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
    objShadowBuilder.renderPass = renderer.getDepthRenderPass();
    objShadowBuilder.hasMultipleInstance = true;

    depthRenderSystem = GlobalRenderSystem::create<Model>(device, objShadowBuilder);
    
    RenderSystemBuilder gltfShadowBuilder{};
    gltfShadowBuilder.vertFilepath = "shaders\\shadowmapgltf.vert.spv";
    gltfShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
    gltfShadowBuilder.renderPass = renderer.getDepthRenderPass(); 

    depthRenderSystemGltf = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, gltfShadowBuilder);

    RenderSystemBuilder terrainBuilder{};
    
    terrainBuilder.vertFilepath = "shaders\\simple_shader.vert.spv";
    terrainBuilder.fragFilepath = "shaders\\terrainShader.frag.spv";
    terrainBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
    terrainBuilder.renderPass = renderer.getSwapChainRenderPass();
    terrainBuilder.hasMultipleInstance = true;
    terrainBuilder.subModelType = TERRAIN;

    terrainRenderSystem = GlobalRenderSystem::create<Model>(device, terrainBuilder);

}

void App::getFrameRate(float lastFrameTime)
{
    float v = 1 / (lastFrameTime * 50);
    frameTimeSum += v;
    frameTimeSum -= frameTimeVector[0];

    frameTimeVector.push_back(v);
    frameTimeVector.erase(frameTimeVector.begin()); 
}
