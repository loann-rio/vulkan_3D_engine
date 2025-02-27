#include "objectManager.h"

void ObjectManager::startLoadModel()
{
    loadObjectAsync(device, "model/2.0/damagedhelmet/gltf/damagedhelmet.gltf", {});
}

void ObjectManager::pushModel(DescriptorPool& pool)
{
    auto it = futureGameObjects.begin(); 
    while (it != futureGameObjects.end()) { 
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) { 
            futureObject object = it->get();  

            auto gameObject = GameObject::createGameObject(device); 
            gameObject.transform = object.transform; 
            gameObject.setModel(object.model); 
            gameObject.setModelType(object.type); 
            gameObject.createDescriptorSet(pool); 

            gameObjects->emplace(gameObject.getId(), std::move(gameObject)); 

            it = futureGameObjects.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ObjectManager::loadObjectAsync(Device& device, const std::string& filePath, TransformComponent transform)
{
    futureGameObjects.push_back(std::async(std::launch::async, [filePath, transform, &device]() {
        std::shared_ptr<GlTFModel::ModelGltf> model = GlTFModel::createModelFromFile(device, filePath);
        return futureObject{ model, transform, GLTF_MODEL }; 
        })
    );
}

void ObjectManager::loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform)
{
    futureGameObjects.push_back(std::async(std::launch::async, [filePath, filePathTexture, transform, &device]() { 
        std::shared_ptr<Model> model = Model::createModelFromFile(device, filePath, filePathTexture);
        return futureObject{ model, transform, OBJ_MODEL }; 
        })
    );
}
