#pragma once

#include <future>
#include <vector>
#include <mutex>
#include <deque>
#include <functional>

#include "GameObject.h"
#include "GameObjectRegistry.h"
#include "../model/Model.h"

struct futureObject {
    ModelVariant model;
    ModelType type;
    GameObject::id_t id;
    std::vector<Model::Instance> instances{};
    bool saveable = true;
};

struct FutureLoadResult {
    std::vector<futureObject> objects;
};

class ModelLoader {
public:
    explicit ModelLoader(Device& device);
    ~ModelLoader();

    // schedule async load; returns a future handle
    std::future<std::vector<futureObject>> loadModelAsync(const std::string& path, const std::string& texturePath = "", TransformComponent transform = {}, const std::string& name = "");

    // schedule primitive generation async
    std::future<std::vector<futureObject>> generatePrimitiveAsync(PrimitivesModelType type, int detail, const std::string& texturePath, GameObject::id_t id);

    // push futures created elsewhere
    void pushFuture(std::future<std::vector<futureObject>> fut);

    // Called each frame/tick from ObjectManager: moves ready futures into out list and returns ready results
    std::vector<std::vector<futureObject>> collectReady();

    // synchronous convenience wrappers
    std::shared_ptr<Model> loadModelSync(const std::string& path, const std::string& texturePath = "");

private:
    Device& device;

    std::mutex mtx;
    std::deque<std::future<std::vector<futureObject>>> pending;
};