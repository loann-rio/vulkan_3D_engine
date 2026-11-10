#pragma once


#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>

struct Scene {
    std::unique_ptr<ModelManager> modelMgr;
    std::unique_ptr<TextureManager> textureMgr;
    std::unique_ptr<GameObjectManager> GameObjectMgr;

    bool opaque = true; // should the lower scene be visible

};


class AssetManager {
public:
    AssetManager() 
    {
        createNewScene();
    };

    
    TextureManager& textures(int targetScene = -1) { 
        if (targetScene == -1) targetScene = currentSceneIndex;
        return *scenes[targetScene]->textureMgr; 
    }

    ModelManager& models(int targetScene = -1) { 
        if (targetScene == -1) targetScene = currentSceneIndex;
        return *scenes[targetScene]->modelMgr;
    }

    GameObjectManager& gameObject(int targetScene = -1) { 
        if (targetScene == -1) targetScene = currentSceneIndex;
        return *scenes[targetScene]->GameObjectMgr;
    }

    //void update();
    void switchScene(uint16_t newSceneIndex) {
        if (newSceneIndex >= scenes.size())
            throw std::runtime_error("could not switch to scene: target does not exist");

        currentSceneIndex = newSceneIndex;
    }

    std::vector<std::unique_ptr<Scene>> scenes{};

private:
    uint16_t currentSceneIndex = 0;


    void createNewScene() {
        auto newScene = std::make_unique<Scene>();

        newScene->textureMgr = std::make_unique<TextureManager>();
        newScene->modelMgr = std::make_unique<ModelManager>();
        newScene->GameObjectMgr = std::make_unique<GameObjectManager>();

        scenes.push_back(std::move(newScene));
    };
};
