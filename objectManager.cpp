#include "objectManager.h"

#include "preBuild.h"

#include <random>

void ObjectManager::startLoadModel(DescriptorPool& pool)
{

    auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f , .1f, 100.f);
    cameraObject->transform.translation = { 2.0f, -1.0f, 2.5f };
    cameraObject->transform.rotation.y = pi<float> * 1 / 3;
    cameraObject->setName("mainCamera");
    pushGameObject(std::move(cameraObject)); 

    std::shared_ptr<Model> plane = createPlane(device, 10, 10, { 0, 0, 0 });
    auto plane1 = GameObjectFactory::createGameObject<GameObjectModel>(device);
    plane1->setModel(plane);
    plane1->transform.translation.y = 0.1f;
    plane1->createDescriptorSet(pool);
    pushGameObject(std::move(plane1));

    TransformComponent vikingRoomTransform{};
    //vikingRoomTransform.rotation = { pi<float> / 2, pi<float>, 0 };
    //vikingRoomTransform.translation = { 7, 0, 7 };
    vikingRoomTransform.scale = { 0.1, 0.1, 0.1 };
    loadObjectAsyncObj(device, "model/viking_room.obj", "textures/viking_room.png", vikingRoomTransform, "viking");

    auto* obj = dynamic_cast<GameObjectModel*>(get("viking"));

    std::vector<Model::Instance> instances{ {} };

    std::random_device rd;
    std::mt19937 gen(rd()); 

    std::uniform_real_distribution<float> dis(0, 100);

    for (int i = 0; i < 10000; i++) {
        instances.push_back({ {dis(gen), -dis(gen), dis(gen)}, {pi<float> / 2, pi<float>, 0}, {1, 1, 1}});
    }

    obj->setMultipleInstances(instances);


     
}

void ObjectManager::pushGameObject(std::unique_ptr<GameObject> gameObject)
{
    GameObject::id_t id = gameObject->getId();
    std::string name = gameObject->getName();
    std::type_index type = typeid(*gameObject);

    gameObjects->emplace(id, std::move(gameObject));

    // Store pointer in name map
    if (!name.empty()) {
        gameObjectsByName[name] = gameObjects->at(id).get();
    }

    // Store in type-indexed list
    gameObjectsByType[type].push_back(gameObjects->at(id).get()); 
}

GameObject* ObjectManager::get(GameObject::id_t id)
{
    auto it = gameObjects->find(id);
    return (it != gameObjects->end()) ? it->second.get() : nullptr;
}

GameObject* ObjectManager::get(const std::string& name)
{
    auto it = gameObjectsByName.find(name);
    return (it != gameObjectsByName.end()) ? it->second : nullptr;
}

void ObjectManager::loadObjectAsync(Device& device, const std::string& filePath, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device); 
    gameObject->transform = transform; 
    gameObject->setName(name.empty() ? filePath : name); 

    futureGameObjects.push_back(std::async(std::launch::async, [filePath, &device]() { 
        std::shared_ptr<GlTFModel::ModelGltf> model = GlTFModel::createModelFromFile(device, filePath);
        return futureObject{ model, model ? GLTF_MODEL : UNDEFINED_MODEL}; 
        })
    );

    pushGameObject(std::move(gameObject));
}

void ObjectManager::loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device); 
    gameObject->transform = transform;  
    gameObject->setName(name.empty() ? filePath : name); 
    GameObject::id_t id = gameObject->getId();

    futureGameObjects.push_back(std::async(std::launch::async, [filePath, filePathTexture, &device, id]() { 
        std::shared_ptr<Model> model = Model::createModelFromFile(device, filePath, filePathTexture);
        return futureObject{ model, model ? OBJ_MODEL : UNDEFINED_MODEL, id };
        })
    );

    pushGameObject(std::move(gameObject)); 
}

/// <summary>
/// take the loaded model from the future and put it in a game object
/// </summary>
/// <param name="pool">global model pool</param>
void ObjectManager::pushModel(DescriptorPool& pool)
{
    auto it = futureGameObjects.begin();
    while (it != futureGameObjects.end()) { // iter over futures
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // check if future is ready
            futureObject object = it->get();  // get loaded model from future

            if (object.type != UNDEFINED_MODEL) {
                auto* gameObject = dynamic_cast<GameObjectModel*>(get(object.id)); 
                gameObject->setModel(object.model); 
                gameObject->setModelType(object.type);
                gameObject->createDescriptorSet(pool); 
            }

            it = futureGameObjects.erase(it); // remove from futures
        }
        else {
            ++it;
        }
    }
}

