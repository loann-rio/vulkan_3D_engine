#pragma once

#include <iostream>
#include <vector>
#include <future>
#include <queue>
#include <mutex>
#include <thread>
#include <typeindex>

#include "GameObject.h"

template<class T>
constexpr T pi = T(3.1415926535897932385L);

struct futureObject {
    ModelVariant model;
    ModelType type;
    GameObject::id_t id; 
    std::vector<Model::Instance> instances{};
}; 

class ObjectManager
{
public:

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    ObjectManager(Device& device) : device{ device } {
        gameObjects = std::make_shared<GameObject::Map>();
    };

	
	void startLoadModel(DescriptorPool& pool); 
	void pushModel(DescriptorPool& pool); 
    void pushGameObject(std::unique_ptr<GameObject> gameObject);

    template <typename T>
    std::vector<T*> getByType();

    GameObject* get(GameObject::id_t id);
    GameObject* get(const std::string& name);

    void removeGameObject(GameObject::id_t id);
    void removeGameObject(const std::string& name) {};

    std::shared_ptr<GameObject::Map> getGameObjects() const { return gameObjects; } 

    void loadObjectAsync(Device& device, const std::string& filePath, TransformComponent transform, const std::string& name = "");
    void loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform, const std::string& name = "");

    void pushFuture(std::future<futureObject> future) { futureGameObjects.push_back(std::move(future)); };
    void pushFuture(std::future<std::vector<futureObject>> futures);

    // camera
    std::string mainCamera = "mainCamera"; 


private:
    Device& device;

    std::mutex gameObjectsMutex;

    std::vector<std::future<futureObject>> futureGameObjects;
    std::vector<std::future<std::vector<futureObject>>> futureGameObjectslist;

    std::shared_ptr<GameObject::Map> gameObjects{};
    std::unordered_map<std::string, GameObject*> gameObjectsByName;
    std::unordered_map<std::type_index, std::vector<GameObject*>> gameObjectsByType; 

    std::shared_ptr<GameObject::Map> spotLights{};
    std::shared_ptr<GameObject::Map> cameras{};
};

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
