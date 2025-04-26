#include "ObjectManager.h"

#include "preBuild.h"

#include <random>

void ObjectManager::startLoadModel(DescriptorPool& pool)
{

    auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f , .1f, 100.f);
    cameraObject->transform.translation = { 3.0f, -1.0f, 3.5f };
    cameraObject->transform.rotation.y = pi<float> * 1 / 3;
    cameraObject->setName("mainCamera");
    pushGameObject(std::move(cameraObject)); 

    auto cameraObject1 = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f, .1f, 100.f); 
    cameraObject1->transform.translation = { 3.0f, -1.0f, 3.5f };
    cameraObject1->transform.rotation.y = pi<float> *1 / 3;
    cameraObject1->setName("camera2");
    pushGameObject(std::move(cameraObject1));


    TransformComponent vikingRoomTransform{};
    vikingRoomTransform.rotation = { pi<float> / 2, pi<float>, 0 };
    vikingRoomTransform.translation = { 7, 0, 7 };
    loadObjectAsyncObj(device, "model/viking_room.obj", "textures/viking_room.png", vikingRoomTransform, "viking");
  
    std::shared_ptr<Model> plane = createPlane(device, 10, 10, {0, 0, 0});
    auto plane1 = GameObjectFactory::createGameObject<GameObjectModel>(device);
    plane1->setModel(plane);
    plane1->transform.translation.y = 0.f;
    plane1->createDescriptorSet(pool);
    pushGameObject(std::move(plane1));

    TransformComponent gltfTransform{};
    gltfTransform.rotation = { -pi<float> / 2, 0, 0 };
    gltfTransform.translation = { 7, 0, 7 };
    //gltfTransform.scale = { 0.1, 0.1, 0.1 };
    loadObjectAsync(device, "model\\2.0\\BrainStem\\glTF\\BrainStem.gltf", gltfTransform, "test gltf");
    //loadObjectAsync(device, "model\\DamagedHelmet.gltf", gltfTransform, "test gltf");
    //loadObjectAsync(device, "model\\2.0\\Sponza\\glTF\\Sponza.gltf", gltfTransform, "test gltf");

    auto spotLight1 = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, glm::radians(50.f), 1.f, .1f, 100.f);
    spotLight1->transform.translation = { -4.0f, -1.0f, 5.5f };
    spotLight1->transform.rotation.y = pi<float> *2 / 5;
    spotLight1->transform.color = { 1.0, 1.0, 1.0, .7 };
    spotLight1->setName("spotLight1");

    auto spotLight2 = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, glm::radians(50.f), 1.f, .1f, 100.f);
    spotLight2->transform.translation = { 1.0f, -2.0f, 2.5f };
    spotLight2->transform.rotation.y = pi<float> *2 / 5;
    spotLight2->transform.color = { 0.0, 1.0, 0.0, .7 };
    spotLight2->setName("spotLight2");

    std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
    auto plane2 = GameObjectFactory::createGameObject<GameObjectModel>(device);
    plane2->setModel(planeModel);
    plane2->transform.rotation.z = -pi<float> / 2;
    plane2->transform.translation.x = 10.f;
    plane2->transform.translation.y = 1.f;
    plane2->createDescriptorSet(pool);
    plane2->setName("plane");
    pushGameObject(std::move(plane2));

    auto plane3 = GameObjectFactory::createGameObject<GameObjectModel>(device);
    plane3->setModel(planeModel);
    plane3->transform.rotation.x = pi<float> / 2;
    plane3->transform.translation.z = 10.f;
    plane3->transform.translation.y = 1.f;
    plane3->createDescriptorSet(pool);
    pushGameObject(std::move(plane3));


    pushGameObject(std::move(spotLight1));
    pushGameObject(std::move(spotLight2));

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
    GameObject::id_t id = gameObject->getId(); 

    pushGameObject(std::move(gameObject));

    futureGameObjects.push_back(std::async(std::launch::async, [filePath, &device, id]() {
        std::shared_ptr<GlTFModel::ModelGltf> model = GlTFModel::createModelFromFile(device, filePath);
        return futureObject{ model, model ? GLTF_MODEL : UNDEFINED_MODEL, id };
        })
    );

    
}

void ObjectManager::loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device); 
    gameObject->transform = transform;  
    gameObject->setName(name.empty() ? filePath : name); 
    GameObject::id_t id = gameObject->getId();

    pushGameObject(std::move(gameObject)); 
     
    futureGameObjects.push_back(std::async(std::launch::async, [filePath, filePathTexture, &device, id]() { 
        std::shared_ptr<Model> model = Model::createModelFromFile(device, filePath, filePathTexture);
        return futureObject{ model, model ? OBJ_MODEL : UNDEFINED_MODEL, id };
        })
    );

    
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

