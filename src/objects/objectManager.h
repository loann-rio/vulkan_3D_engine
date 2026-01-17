#pragma once

#include <iostream>

#include <vector>
#include <future>
#include <typeindex>

#include "GameObject.h"
#include "../render/Renderer.h"
#include "../render/GlobalRenderSystem.h" 

#include "../assetManager/AssetManager.h"

#include "../model/ModelAsset.h"


#include <../json.hpp>
using json = nlohmann::json;


/// <summary>
/// Represents a future game object with associated model information and instances.
/// </summary>
struct futureObject {
    ModelVariant model;
    ModelType type;
    GameObject::id_t id; 
    std::vector<Model::Instance> instances{};
    bool saveable = true;
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

    template <typename T>
    std::vector<T*> getByType();

    GameObject* get(const GameObject::id_t id);
    GameObject* get(const std::string& name);

    void removeGameObject(const GameObject::id_t id);
    void removeGameObject(const std::string& name);
    void removeGameObject(GameObject* gameObject);

    std::shared_ptr<GameObject::Map> getGameObjects() const { return gameObjects; } 

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

    void generateSkybox(const std::string pathTexture, const std::string goName, Renderer* renderer, std::shared_ptr<GlobalRenderSystem> skyboxRenedrSystem);

    // camera
    std::string mainCamera = "mainCamera"; 

    // main skybox
    std::string mainSkybox = "cubemap";

    AssetManager& assetManager;
private:
    Device& device;
    

    std::vector<std::future<futureObject>> futureGameObjects;
    std::vector<std::future<std::vector<futureObject>>> futureGameObjectslist;

	// main storage
    std::shared_ptr<GameObject::Map> gameObjects{};
    std::unordered_map<std::string, GameObject*> gameObjectsByName;
    std::unordered_map<std::type_index, std::vector<GameObject*>> gameObjectsByType; 

    void addObjectToScene(GameObject* gameObject);

    std::unique_ptr<DescriptorPool> globalPool{};

    void createDescriptorSet(ModelAsset* model);
};




/// <summary>
/// Retrieves all managed objects of a specified type.
/// </summary>
/// <typeparam name="T">The type of objects to retrieve.</typeparam>
/// <returns>A vector containing pointers to objects of type T managed by ObjectManager.</returns>
template<typename T>
inline std::vector<T*> ObjectManager::getByType()
{
    std::vector<T*> result; 
    auto it = gameObjectsByType.find(typeid(T));  

    if (it != gameObjectsByType.end()) { 
        for (auto obj : it->second) { 
            if (T* casted = dynamic_cast<T*>(obj)) { 
                result.push_back(casted);
            }
        }
    }
    
    return result;  
}
