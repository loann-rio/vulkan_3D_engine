#pragma once

#include <vector>
#include <future>
#include <typeindex>
#include <cstdint>
#include <../json.hpp>

#include "GameObject.h"

#include "../assetManager/AssetManager.h"
#include "../model/ModelAsset.h"
#include "../base/descriptors.h"
#include "../Scene/SceneLoader.h"

using json = nlohmann::json;

class GameObject;

/// <summary>
/// Represents a future game object with associated model information and instances.
/// </summary>
struct futureObject {
    ModelType type;
    GameObject::id_t id; 
    std::vector<ModelInstance> instances{};
    bool saveable = true;
    ModelManager::ModelID modelID;
    std::string targetScene;
}; 

struct ComputeObject {
    bool enable{ false };
    uint32_t groupX{ 1 };
    uint32_t groupY{ 1 };
    uint32_t groupZ{ 1 };
    size_t instanceCount{ 0 };
    GameObject* gameObject;
};

class ObjectManager
{
public:

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    ObjectManager(Device& device, AssetManager& assetManager);

    ~ObjectManager() { globalPool = nullptr; }
	
	void loadHardcodedObjects(); 
	void updateObjectManager();
    void pushGameObject(std::unique_ptr<GameObject> gameObject, const std::string& targetScene = "");

	void createPrimitive(PrimitivesModelType type, int detail, TransformComponent transform, const std::string& name = "", const std::string& filePathTexture = "", const std::string& targetScene = "");

    void loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, TransformComponent transform, const std::string& name = "", const std::string& targetScene = "");
    void loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, const std::string filePathTexture, TransformComponent transform, const std::string& name = "", const std::string& targetScene = "");

    void pushFuture(std::future<std::vector<futureObject>> futures);


    void requestSwitchScene(std::string newScene) { newSceneName = newScene; }
    DescriptorPool* getPool() const { return globalPool.get(); }

    std::string mainCamera = "mainCamera"; 
    std::string mainSkybox = "cubemap";
    std::string currentScene = "test5";

    void createDescriptorSet(ModelAsset* model, const std::string& targetScene);

    std::vector<ComputeObject> computeList{};

private:
    std::string newSceneName = "";

    Device& device;
    AssetManager& assetManager;

    std::vector<std::future<std::vector<futureObject>>> futureGameObjectslist;

    std::unique_ptr<DescriptorPool> globalPool{};    
};