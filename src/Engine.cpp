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
#include <cstdint>
#include <memory>
#include <iostream>
#include <glm/fwd.hpp>

Engine::Engine() { 
    objectManager.startLoadModel(); 
    renderer.generateSkybox("assets/skybox/citrus_orchard_puresky_4k.hdr", "testSkybox", objectManager);
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

    auto* textureObject = dynamic_cast<GameObjectModel*>(assetManager.gameObject().findByName("cubemap1"));

    device.waitQueueIdle(device.presentQueue());
	while (!window.shouldClose())
	{   
		glfwPollEvents();

        // add loaded async model to gameObjectmap 
        objectManager.updateObjectManager();

        // calculate frame time
        auto newTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        /////// start frame ///////       
        int frameIndex = renderer.getFrameIndex();

        /////// update objects ///////

        // loop on games objects
        assetManager.gameObject().update(frameTime);        

        // update camera
        auto camera = dynamic_cast<GameObjectCamera*>(assetManager.gameObject("fix_elements").findByName(objectManager.mainCamera));

        {
            if (!renderer.isUiSelected())
                cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, camera);
            camera->updateCameraView();

            if (camera->camera->aspectRatio != renderer.getAspectRatio()) {
                std::cout << "wrong aspect ratio \n";
                camera->camera->setPerspectiveProjection(renderer.getAspectRatio());
            }

            ubo.projection = camera->camera->getProjection();
            ubo.view = camera->camera->getView();
            ubo.lightPos = glm::vec3(camera->getTransformMat()[3]);
        }

        // update pointLight 
        {
            std::vector<GameObjectPointLight*> pointLigths = assetManager.gameObject().getByType<GameObjectPointLight>();

            uint16_t i = 0;
            for (auto lightObj : pointLigths) {
                if (lightObj->transform.color.w)
                    ubo.pointLights[i++] = PointLight{ glm::vec4(lightObj->transform.translation, lightObj->transform.scale.x), lightObj->transform.color };
                if (i >= MAX_LIGHT) break;
            }
            ubo.numLights = i;
        }

        renderer.waitForFrame();

        renderer.uboBuffers[frameIndex]->writeToBuffer(&ubo); 
        renderer.uboBuffers[frameIndex]->flush();

        std::vector<std::array<FrustumPlane, 6>> frustrumPlanesList;
        // update spotLight
        {
            uint16_t i = 0; 
            std::vector<GameObjectSpotLight*> spotLigths = assetManager.gameObject().getByType<GameObjectSpotLight>();
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
            0,
            frameTime,
            spotLightUbo.numLights,
            assetManager.gameObject("fix_elements").findByName(objectManager.mainCamera)->transform.translation,
            assetManager.gameObject().getByType<GameObjectModel>(),
            std::vector<ComputeObject*>{},
            frustrumPlanesList,
            camera->getFrustumPlanes(),
        };

        /////// render frame ///////
        renderer.renderFrame(frameInfo, objectManager);
	}

    device.waitQueueIdle(device.presentQueue());
}
