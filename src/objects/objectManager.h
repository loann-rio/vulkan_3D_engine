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

    ~ObjectManager() { saveFullScene(); globalPool = nullptr; }
	
	void startLoadModel(); 
	void pushModel();
    void pushGameObject(std::unique_ptr<GameObject> gameObject);

	// create primitive object
	void createPrimitive(PrimitivesModelType type, int detail, TransformComponent transform, const std::string& name = "", const std::string& filePathTexture = "");

    void loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, TransformComponent transform, const std::string& name = "");
    void loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, const std::string filePathTexture, TransformComponent transform, const std::string& name = "");

    void pushFuture(std::future<std::vector<futureObject>> futures);

    // scene 
	json currentSceneJson;
    std::string currentScene = "test5";
	std::string scenePath = "scenes/test5.json";

    void switchScene(std::string name); 
    void loadScene(std::string name);
    void createScene(std::string name);
	void saveFullScene();

    DescriptorPool* getPool() const { return globalPool.get(); }

    // camera
    std::string mainCamera = "mainCamera"; 
    std::string mainSkybox = "cubemap";


    void createDescriptorSet(ModelAsset* model);

    std::vector<ComputeObject> computeList{};
    GameObjectModel* baseSkyBox = nullptr;
private:
    Device& device;
    AssetManager& assetManager;

    std::vector<std::future<futureObject>> futureGameObjects;
    std::vector<std::future<std::vector<futureObject>>> futureGameObjectslist;

    void addObjectToScene(GameObject* gameObject);

    std::unique_ptr<DescriptorPool> globalPool{};    
};