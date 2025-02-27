#pragma once

#include <iostream>
#include <vector>
#include <future>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>

#include "GameObject.h"

struct futureObject {
    ModelVariant model;
    TransformComponent transform;
    ModelType type;
};

class ObjectManager
{
public:

    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;

    ObjectManager(Device& device) : device{ device } {
        gameObjects = std::make_shared<GameObject::Map>();
    };


	std::shared_ptr<GameObject::Map> getGameObject() const { return gameObjects; }
	std::shared_ptr<GameObject::Map> getSpotLights() const { return spotLights;  }

	void startLoadModel();
	void pushModel(DescriptorPool& pool); 

private:
    Device& device;

    std::mutex gameObjectsMutex;

    std::vector<std::future<futureObject>> futureGameObjects;

    std::shared_ptr<GameObject::Map> spotLights{};
    std::shared_ptr<GameObject::Map> gameObjects{};
    std::shared_ptr<GameObject::Map> cameras{};

    void loadObjectAsync(Device& device, const std::string& filePath, TransformComponent transform);
    void loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform);
};
 