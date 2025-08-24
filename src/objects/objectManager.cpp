#include "ObjectManager.h"

#include "../model/preBuild.h"

#include "../base/VoronoiNoise.h"

#include "../GameObjectClassAssets/TerrainGenerator.h"
#include "../GameObjectClassAssets/mainLightBehavior.h"

#include <random>
#include <chrono>
#include <stdlib.h>


void ObjectManager::startLoadModel(DescriptorPool& pool)
{

    auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f , .1f, 100.f);
    cameraObject->transform.translation = { 0.f, -3.0f, 3.f };
    cameraObject->transform.rotation.y = pi<float> * 1 / 3;
    cameraObject->setName("mainCamera");
    pushGameObject(std::move(cameraObject)); 

    auto spotLight1 = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, glm::radians(50.f), 1.f, .1f, 100.f);
    spotLight1->transform.translation = { -5.f, -10.0f, -7.6f };
    spotLight1->transform.rotation = { -.07, 0.67, -0.358 };
    spotLight1->camera->_fov = 1.5;
    spotLight1->transform.color = { 1.0, 1.0, 1.0, .7 };
    spotLight1->setName("spotLight1");
    pushGameObject(std::move(spotLight1));

    auto terrainManager = GameObjectFactory::createGameObject<GameObject>(device);
    terrainManager->setAttachedClass(std::make_unique<TerrainGenerator>(device));
    terrainManager->setName("terrain G");
    pushGameObject(std::move(terrainManager));


    
   
    /*TerrainGenerator tg{ 9876, device };

    srand(1234567);


    for (int i = 0; i < tg.sizeWorldInChunck; i++) 
    for (int j = 0; j < tg.sizeWorldInChunck; j++) 
    {
        loadChunckAsync(device, &tg, i * (tg.chunkSize - 1), j * (tg.chunkSize - 1));
    }*/


    //std::cout << "nb trees : " << treeList.size() << "\n";

    /*std::shared_ptr<Model> trees = Model::createModelFromFile(device, "C:\\Users\\riolo\\Desktop\\vulkan_3D_engine\\model\\coloredTree1.obj", "textures\\whiteTexture.jpg");

    auto treesObject = GameObjectFactory::createGameObject<GameObjectModel>(device);
    treesObject->setModel(trees);
    treesObject->createDescriptorSet(pool);
    treesObject->setName("trees");
    treesObject->setMultipleInstances(treeList);
    pushGameObject(std::move(treesObject));*/

}

void ObjectManager::pushGameObject(std::unique_ptr<GameObject> gameObject)
{
    GameObject::id_t id = gameObject->getId();
    std::string name = gameObject->getName();
    std::type_index type = typeid(*gameObject);
    gameObject->setup(this);

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

void ObjectManager::removeGameObject(GameObject::id_t id)
{
    auto it = gameObjects->find(id);
    if (it == gameObjects->end()) {
        return; // nothing to remove
    }

    GameObject* obj = it->second.get();

    // Remove from name map
    if (!obj->getName().empty()) {
        gameObjectsByName.erase(obj->getName());
    }

    // Remove from type-indexed list
    std::type_index type = typeid(*obj);
    auto& vec = gameObjectsByType[type];
    vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());

    // remove from main storage
    gameObjects->erase(it);
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

void ObjectManager::pushFuture(std::future<std::vector<futureObject>> futures)
{
    futureGameObjectslist.push_back(std::move(futures));
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

                
                if (!gameObject) {
                    it = futureGameObjects.erase(it);
                    continue;
                }

                if (object.instances.size() > 0) {
                    gameObject->setMultipleInstances(object.instances);
                }

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



    auto it2 = futureGameObjectslist.begin();
    while (it2 != futureGameObjectslist.end()) { // iter over futures
        if (it2->wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // check if future is ready
            
            for (futureObject object : it2->get()) {  // get loaded model from future

                if (object.type != UNDEFINED_MODEL) {
                    auto* gameObject = dynamic_cast<GameObjectModel*>(get(object.id));


                    if (!gameObject) {
                        it2 = futureGameObjectslist.erase(it2);
                        continue;
                    }

                    if (object.instances.size() > 0) {
                        gameObject->setMultipleInstances(object.instances);
                    }

                    gameObject->setModel(object.model);
                    gameObject->setModelType(object.type);
                    gameObject->createDescriptorSet(pool);
                }
            }

            it2 = futureGameObjectslist.erase(it2); // remove from futures
        }
        else {
            ++it2;
        }
    }
}

