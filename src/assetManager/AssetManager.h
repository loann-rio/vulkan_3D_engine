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
        createNewScene("default");
    };

    
    TextureManager& textures(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        return *scenes[targetScene]->textureMgr; 
    }

    ModelManager& models(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        return *scenes[targetScene]->modelMgr;
    }

    GameObjectManager& gameObject(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        return *scenes[targetScene]->GameObjectMgr;
    }

    //void update();
    void switchScene(std::string newSceneName) {
        if (scenes.find(newSceneName) == scenes.end())
            throw std::runtime_error("could not switch to scene: target does not exist");

        currentSceneName = newSceneName;
    }

    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes{};
    
    void createNewScene(std::string sceneName) {
        auto newScene = std::make_unique<Scene>();

        newScene->textureMgr = std::make_unique<TextureManager>();
        newScene->modelMgr = std::make_unique<ModelManager>();
        newScene->GameObjectMgr = std::make_unique<GameObjectManager>();

        scenes[sceneName] = std::move(newScene);
    };

private:
    std::string currentSceneName = "default";
};
