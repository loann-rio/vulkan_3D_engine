#include "Engine.h"

// local
#include "base/Buffer.h"
#include "base/Frame_info.h"
#include "objects/KeyboardMovementController.h"
#include "render/Camera.h"

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <array>
#include <chrono>
#include <string>
#include <thread>


Engine::Engine() { 
    objectManager.startLoadModel(); 
    renderer.generateSkybox("skybox/citrus_orchard_puresky_4k.hdr", "testSkybox", objectManager);
}


void Engine::run()
{
    // user inputs
    KeyboardMovementController cameraController{};

    // UBO
    GlobalUbo ubo{};
    SpotLightUbo spotLightUbo{};
    TerrainUbo terrainUbo{};

    auto currentTime = std::chrono::high_resolution_clock::now();
	float frameTime = 0.1f;

    auto* textureObject = dynamic_cast<GameObjectModel*>(objectManager.get("cubemap1"));

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
            if (!renderer.isUiSelected())
                cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, objectManager.get(objectManager.mainCamera));
            dynamic_cast<GameObjectCamera*>(objectManager.get(objectManager.mainCamera))->updateCameraView();
        }

        /////// start frame ///////        
        int frameIndex = renderer.getFrameIndex();

        /////// update objects ///////

        // loop on games objects
        objectManager.updateGameObject(frameTime);        

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

        renderer.uboBuffers[frameIndex]->writeToBuffer(&ubo); 
        renderer.uboBuffers[frameIndex]->flush();

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
                if (i >= DepthPass::MAX_DEPTH_RENDER_COUNT) break;
            }
            spotLightUbo.numLights = i;

            renderer.shadowUboBuffer[frameIndex]->writeToBuffer(&spotLightUbo);
            renderer.shadowUboBuffer[frameIndex]->flush();
        }

        FrameInfo frameInfo{
            frameIndex,
            frameTime,
            spotLightUbo.numLights,
            objectManager.get(objectManager.mainCamera)->transform.translation,
            objectManager.getByType<GameObjectModel>(),
            frustrumPlanesList,
            dynamic_cast<GameObjectCamera*>(objectManager.get(objectManager.mainCamera))->getFrustumPlanes(),
        };

        /////// render frame ///////
        renderer.renderFrame(frameInfo, objectManager);
	}

    vkQueueWaitIdle(device.presentQueue());
}
