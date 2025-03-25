#include "App.h"

// local
#include "KeyboardMovementController.h"
#include "Frame_info.h"
#include "preBuild.h"

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// imgui
/*#define ENABLE_IMGUI

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#endif // ENABLE_IMGUI*/

// std
#include <chrono>

#include <iostream>
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
        spotLightUbo.spotLight[i++] = kv.second.getSpotLightInfo(true);
        if (i > DepthSwapChain::MAX_DEPTH_RENDER_COUNT) break;
    }

    spotLightUbo.numLights = i;
    
    shadowUboBuffer[0]->writeToBuffer(&spotLightUbo);
    shadowUboBuffer[0]->flush();

    // start timer
    auto currentTime = std::chrono::high_resolution_clock::now();

    int frame = 0;
	while (!window.shouldClose())
	{
		glfwPollEvents();

        armControler.updateAnglesOnMsg(objectManager.getGameObject()); 
        armControler.sendMousePosition(window.getMousePos(), cameraObject.camera->getView(), cameraObject.camera->getProjection(), cameraController.isSpacePressed);

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

        frame = (frame + 1) % 1; 
        
	}

    vkQueueWaitIdle(device.presentQueue());
}
 
void App::loadGameObjects() {

    objectManager.startLoadModel();

    TransformComponent base{};
    base.translation = { 7, 0, 7 };
    base.scale = { 0.1f, 0.1f, 0.1f };

    TransformComponent link1{};
    link1.translation = { 7, -0.3, 7 };
    link1.rotation = { pi<float> , 0, 0 };
    link1.scale = { 0.1f, 0.1f, 0.1f };

    TransformComponent link2{};
    link2.translation = { 7, -0.55, 7 };
    link2.rotation = { -pi<float> / 2, pi<float> / 2, 0 };
    link2.scale = { 0.1f, 0.1f, 0.1f };

    TransformComponent link3{};
    link3.translation = { 7, -0.55 - 1.8, 7 };
    link3.rotation = { 0, 0, pi<float> / 2 };
    link3.scale = { 0.1f, 0.1f, 0.1f };

    TransformComponent gripper{};
    gripper.translation = { 7, -0.55 - 1.8, 7 - 1.6 };
    gripper.rotation = { pi<float>, 0, 0 };
    gripper.scale = { 0.1f, 0.1f, 0.1f };

    std::shared_ptr<Model> modelBase    = Model::createModelFromFile(device, "roboticArm/base.obj"   , "textures/viking_room.png" );
    std::shared_ptr<Model> modelLink1   = Model::createModelFromFile(device, "roboticArm/link1.obj"  , "textures/emptyTexture.jpg");
    std::shared_ptr<Model> modelLink2   = Model::createModelFromFile(device, "roboticArm/link2.obj"  , "textures/viking_room.png" );
    std::shared_ptr<Model> modelLink3   = Model::createModelFromFile(device, "roboticArm/link3b.obj" , "textures/emptyTexture.jpg");
    std::shared_ptr<Model> modelGripper = Model::createModelFromFile(device, "roboticArm/gripper.obj", "textures/viking_room.png" );

    auto baseGO = GameObject::createGameObject(device); 
    baseGO.setModel(modelBase); 
    baseGO.transform = base; 
    baseGO.createDescriptorSet(*globalPool); 
    
    auto link1GO = GameObject::createGameObject(device); 
    link1GO.setModel(modelLink1); 
    link1GO.transform = link1;
    link1GO.createDescriptorSet(*globalPool); 

    auto link2GO = GameObject::createGameObject(device); 
    link2GO.setModel(modelLink2); 
    link2GO.transform = link2;
    link2GO.createDescriptorSet(*globalPool); 
    
    auto link3GO = GameObject::createGameObject(device); 
    link3GO.setModel(modelLink3); 
    link3GO.transform = link3;
    link3GO.createDescriptorSet(*globalPool); 

    auto gripperGO = GameObject::createGameObject(device);
    gripperGO.setModel(modelGripper);
    gripperGO.transform = gripper;
    gripperGO.createDescriptorSet(*globalPool);

    armControler.setIDObjects(link1GO.getId(), link2GO.getId(), link3GO.getId(), gripperGO.getId());

    objectManager.pushSyncGameObject(std::move(baseGO));
    objectManager.pushSyncGameObject(std::move(link1GO));
    objectManager.pushSyncGameObject(std::move(link2GO)); 
    objectManager.pushSyncGameObject(std::move(link3GO));
    objectManager.pushSyncGameObject(std::move(gripperGO)); 



    std::shared_ptr<Model> plane = createPlane(device, 10, 50, { 0, 0, 0 }, "textures/emptyTexture.jpg");
    auto plane1 = GameObject::createGameObject(device);
    plane1.setModel(plane);
    //plane1.transform.translation.y = 0.1f;
    plane1.createDescriptorSet(*globalPool);
    objectManager.pushSyncGameObject(std::move(plane1));

    /*std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
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
    objectManager.pushSyncGameObject(std::move(plane3));*/

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
    spotLight2.transform.translation = { 6.0f, -8.0f, 6.0f };
    spotLight2.transform.rotation.x = - pi<float> / 2;
    spotLight1.transform.rotation.y = pi<float> *2 / 5;
    spotLight2.transform.color = { 1.0, 1.0, 0.0, .2 };

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
    shadowUboBuffer.resize(1); 
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
        auto bufferInfo = shadowUboBuffer[0]->descriptorInfo();
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
