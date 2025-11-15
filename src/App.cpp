#include "App.h"

// local
#include "base/Buffer.h"
#include "base/Frame_info.h"
#include "objects/Texture.h"
#include "objects/KeyboardMovementController.h"
#include "render/Camera.h"
#include "model/GlTFModel.h"
#include "base/FrameRateCounter.h"
#include "model/preBuild.h"

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
#include <thread>
#include <iomanip>


App::App() { 
    objectManager.startLoadModel(); 
    createRenderSystems();
}


void App::run()
{
    // ui
    BasicUI imgui{ device, window.getGLFWwindow(), renderer.getSwapChainRenderPass() };

    //TextOverlay textOverlay(device, renderer.getSwapChainRenderPass());
    //textOverlay.prepareResources(*globalPool);

    objectManager.generateSkybox("skybox/citrus_orchard_puresky_4k.hdr", "testSkybox", &renderer, skyboxCreationRenderSystem);

    // user inputs
    KeyboardMovementController cameraController{};

    // UBO
    GlobalUbo ubo{};
    SpotLightUbo spotLightUbo{};
    TerrainUbo terrainUbo{};
    
    // frame counter
    FrameRateCounter gpuFrameRate;
    FrameRateCounter cpuFrameRate;


    auto currentTime = std::chrono::high_resolution_clock::now();
	float frameTime = 0.1f;
	float gpuTime = 0.0f;

    int frame = 0;

    auto* textureObject = dynamic_cast<GameObjectModel*>(objectManager.get("cubemap"));

    vkQueueWaitIdle(device.presentQueue());
	while (!window.shouldClose())
	{   
		glfwPollEvents();

        // add loaded async model to gameObjectmap 
        objectManager.pushModel();

        // calculate frame time
        auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
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

        /* {
            // show fps count on screen
            
            cpuFrameRate.update(frameTime);

            std::stringstream ss("");
            ss << std::fixed << std::setprecision(2) << cpuFrameRate.get() << " fps";

            textOverlay.beginTextUpdate(frameIndex);
            textOverlay.addText(frameIndex, ss.str(), 10, 10, TextOverlay::alignLeft, renderer.getWidth(), renderer.getHeight()); 
            textOverlay.endTextUpdate(frameIndex); 
        }*/

        /////// update objects ///////

        // loop on games objects
        {
            std::shared_ptr<GameObject::Map> objects = objectManager.getGameObjects();
            std::vector<GameObject::id_t> toRemove;

            for (auto& [id, obj] : *objects) {
                if (!obj->toBeRemoved) {
                    obj->loop(&objectManager);
                    continue;
                }

                // Handle removal
                if (obj->getType() == GameObjectType::MODEL) {
                    auto* modelObj = dynamic_cast<GameObjectModel*>(obj.get());
                    if (modelObj->show) {
                        modelObj->show = false; // Hide first to avoid descriptor issues
                        continue;
                    }
                }

                // Either not a model, or already hidden — safe to remove
                toRemove.push_back(id);
            }

            // Remove after iteration
            for (auto id : toRemove) {
                objectManager.removeGameObject(id);
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


        // update terrain ubo
        {
            terrainBuffers[frameIndex]->writeToBuffer(&terrainUbo);
            terrainBuffers[frameIndex]->flush();
		}

        std::vector<std::array<FrustumPlane, 6>> frustrumPlanesList;
        // update spotLight
        {
            uint16_t i = 0; 
            std::vector<GameObjectSpotLight*> spotLigths = objectManager.getByType<GameObjectSpotLight>();
            for (auto lightObj : spotLigths) {
                if (lightObj->transform.color.w != 0)
                    spotLightUbo.spotLight[i++] = lightObj->getSpotLightInfo(true);

                lightObj->camera->updateFrustrumPlanes();
                frustrumPlanesList.push_back(lightObj->getFrustumPlanes());
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
            objectManager.getByType<GameObjectModel>(),
            frustrumPlanesList
        };

		
        /////// render frame ///////
        {
            std::vector<VkDescriptorSet> descriptorSets{ globalDescriptorSet[frameIndex], shadowDescriptorSet[renderer.getDepthIndex()]};
			std::array<FrustumPlane, 6> frustrumPlanes = dynamic_cast<GameObjectCamera*>(objectManager.get(objectManager.mainCamera))->getFrustumPlanes();

            vkQueueWaitIdle(device.presentQueue());

            auto newGpuTime = std::chrono::high_resolution_clock::now();
           
			// render shadow map
            renderer.renderDepthImage(frameInfo, { depthRenderSystem, depthRenderSystemGltf, depthTerrainRenderSystem }, descriptorSets);

            if (auto commandBuffer = renderer.beginFrame()) {
                 
                // render
                renderer.beginSwapChainRenderPass(commandBuffer); 

                gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo,
                {
                    globalDescriptorSet[frameIndex],
                    shadowDescriptorSet[renderer.getDepthIndex()],
                    textureObject->getDescriptorSets()[frameIndex]
                },
                frustrumPlanes);
                
                
                objRenderSystem->renderGameObjects(commandBuffer, frameInfo, descriptorSets); 

                std::vector<VkDescriptorSet> terrainDescriptorSets{ globalDescriptorSet[frameIndex], shadowDescriptorSet[renderer.getDepthIndex()], terrainDescriptorSet[frameIndex] };
                terrainRenderSystem->renderGameObjects(commandBuffer, frameInfo, terrainDescriptorSets);

				skyboxRenderSystem->renderGameObjects(commandBuffer, frameInfo, { globalDescriptorSet[frameIndex] });

                //textOverlay.renderText(commandBuffer, frameInfo); 

                imgui.drawUI(commandBuffer, &objectManager, terrainUbo, gpuFrameRate.get());

                renderer.endSwapChainRenderPass(commandBuffer); 
                renderer.endFrame();
            } 

			auto endGpuTime = std::chrono::high_resolution_clock::now();
			gpuTime = std::chrono::duration<float, std::chrono::seconds::period>(endGpuTime - newGpuTime).count();
			gpuFrameRate.update(gpuTime);
        }   
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

        DescriptorWriter(*globalSetLayout, *objectManager.getPool())
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]); 
    } 

    //// terrain buffer 

    terrainBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < terrainBuffers.size(); i++)
    {
        terrainBuffers[i] = std::make_unique<Buffer>(
            device,
            sizeof(TerrainUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            device.properties.limits.minUniformBufferOffsetAlignment
        );

        terrainBuffers[i]->map();
    }

    auto terrainSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    terrainDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < terrainDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = terrainBuffers[i]->descriptorInfo();

        DescriptorWriter(*terrainSetLayout, *objectManager.getPool())
            .writeBuffer(0, &bufferInfo)
            .build(terrainDescriptorSet[i]);
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

        DescriptorWriter(*shadowSetLayout, *objectManager.getPool())
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, depthInfo, DepthSwapChain::MAX_DEPTH_RENDER_COUNT)
            .build(shadowDescriptorSet[i]); 
    }


    /// skybox 
    auto skyboxSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) 
        .build();

    /// render systems

    {
        {
            RenderSystemBuilder gltfBuilder{};
            gltfBuilder.fragFilepath = "shaders\\GlTFshader.frag.spv";
            gltfBuilder.vertFilepath = "shaders\\GlTFshader.vert.spv";
            gltfBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout(), skyboxSetLayout->getDescriptorSetLayout() };
            gltfBuilder.renderPass = renderer.getSwapChainRenderPass();

            gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, gltfBuilder);
        }

        {
            RenderSystemBuilder gltfShadowBuilder{};
            gltfShadowBuilder.vertFilepath = "shaders\\shadowmapgltf.vert.spv";
            gltfShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
            gltfShadowBuilder.renderPass = renderer.getDepthRenderPass();

            depthRenderSystemGltf = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, gltfShadowBuilder);
        }
    }

    {
        {
            RenderSystemBuilder objBuilder{};
            objBuilder.fragFilepath = "shaders\\simple_shader.frag.spv";
            objBuilder.vertFilepath = "shaders\\simple_shader.vert.spv";
            objBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
            objBuilder.renderPass = renderer.getSwapChainRenderPass();
            objBuilder.hasMultipleInstance = true;

            objRenderSystem = GlobalRenderSystem::create<Model>(device, objBuilder);
        }

        {
            RenderSystemBuilder objShadowBuilder{};
            objShadowBuilder.vertFilepath = "shaders\\shadowmap.vert.spv";
            objShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
            objShadowBuilder.renderPass = renderer.getDepthRenderPass();
            objShadowBuilder.hasMultipleInstance = true;

            depthRenderSystem = GlobalRenderSystem::create<Model>(device, objShadowBuilder);
        }
    }
    
    

    {
        {
            RenderSystemBuilder terrainBuilder{};

            terrainBuilder.vertFilepath = "shaders\\terrainShader.vert.spv";
            terrainBuilder.fragFilepath = "shaders\\terrainShader.frag.spv";
            terrainBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() , terrainSetLayout->getDescriptorSetLayout() };
            terrainBuilder.renderPass = renderer.getSwapChainRenderPass();
            terrainBuilder.hasMultipleInstance = true;
            terrainBuilder.subModelType = ModelSubType::TERRAIN;

            terrainRenderSystem = GlobalRenderSystem::create<Model>(device, terrainBuilder);
        }

        {
            RenderSystemBuilder terrainShadowBuilder{};
            terrainShadowBuilder.vertFilepath = "shaders\\shadowMapTerrain.vert.spv";
            terrainShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
            terrainShadowBuilder.renderPass = renderer.getDepthRenderPass();
            terrainShadowBuilder.hasMultipleInstance = true;
            terrainShadowBuilder.subModelType = ModelSubType::TERRAIN;

            depthTerrainRenderSystem = GlobalRenderSystem::create<Model>(device, terrainShadowBuilder);
        }
    }

    {
        RenderSystemBuilder skyboxBuilder{};
        skyboxBuilder.fragFilepath = "shaders\\skybox.frag.spv";
        skyboxBuilder.vertFilepath = "shaders\\skybox.vert.spv";
        skyboxBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout() };
        skyboxBuilder.renderPass = renderer.getSwapChainRenderPass();
        skyboxBuilder.subModelType = ModelSubType::SKYBOX;
        skyboxBuilder.isSkyBox = true;
        skyboxRenderSystem = GlobalRenderSystem::create<Model>(device, skyboxBuilder);
    }

    {
        RenderSystemBuilder skyboxBuilder{};
        skyboxBuilder.fragFilepath = "shaders\\equirectangular_to_cube.frag.spv";
        skyboxBuilder.vertFilepath = "shaders\\fullscreen.vert.spv";
        skyboxBuilder.renderPass = renderer.getSecondarySwapRenderPass();
        skyboxBuilder.isFullscreenRender = true;
        skyboxBuilder.pushStage = static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_FRAGMENT_BIT);
        skyboxCreationRenderSystem = GlobalRenderSystem::create<Model>(device, skyboxBuilder);
    }
}
