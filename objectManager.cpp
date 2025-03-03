#include "objectManager.h"

void ObjectManager::startLoadModel()
{
    TransformComponent helmetTransform{};
    helmetTransform.rotation = { 3 * pi<float> / 2, pi<float>, 0 };
    helmetTransform.translation = { 8, -0.5, 9 };
    helmetTransform.scale = { 0.5f, 0.5f, 0.5f };
    loadObjectAsync(device, "model/2.0/damagedhelmet/gltf/damagedhelmet.gltf", helmetTransform);

    TransformComponent vikingRoomTransform{};
    vikingRoomTransform.rotation = { pi<float> / 2, pi<float>, 0 };
    vikingRoomTransform.translation = { 7, 0, 7 };
    loadObjectAsyncObj(device, "model/viking_room.obj", "textures/viking_room.png", vikingRoomTransform);
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

            auto gameObject = GameObject::createGameObject(device);  
            gameObject.transform = object.transform; 
            gameObject.setModel(object.model); 
            gameObject.setModelType(object.type); 
            gameObject.createDescriptorSet(pool); 

            gameObjects->emplace(gameObject.getId(), std::move(gameObject)); 

            it = futureGameObjects.erase(it); // remove from futures
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
