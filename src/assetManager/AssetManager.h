#pragma once


#include "ModelManager.h"
#include "TextureManager.h"
#include "GameObjectManager.h"
#include <memory>
#include <stdexcept>
#include <utility>
#include <string>

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
        createNewScene("fix_elements");
    };

    ~AssetManager(){
        scenes.clear();
    };

    
    TextureManager& textures(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        TextureManager& textureMgr = *scenes[targetScene]->textureMgr;
        textureMgr.setCurrentTargetScene(targetScene);
        return textureMgr; 
    }

    ModelManager& models(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        ModelManager& modelMgr = *scenes[targetScene]->modelMgr;
        modelMgr.setCurrentTargetScene(targetScene);
        return modelMgr;
    }

    GameObjectManager& gameObject(std::string targetScene = "") { 
        if (targetScene == "") targetScene = currentSceneName;
        GameObjectManager& gameObjectMgr = *scenes[targetScene]->GameObjectMgr;
        gameObjectMgr.setCurrentTargetScene(targetScene);
        return gameObjectMgr;
    }

    void switchScene(std::string newSceneName) {

        if (scenes.find(newSceneName) == scenes.end())
		    createNewScene(newSceneName);
        
        currentSceneName = newSceneName;
    }

    
    void createNewScene(std::string sceneName) {
        auto newScene = std::make_unique<Scene>();

        newScene->textureMgr = std::make_unique<TextureManager>();
        newScene->modelMgr = std::make_unique<ModelManager>();
        newScene->GameObjectMgr = std::make_unique<GameObjectManager>();

        scenes[sceneName] = std::move(newScene);
    };

private:
    std::string currentSceneName = "default";

    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes{};

};
