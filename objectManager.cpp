#include "objectManager.h"

#include "preBuild.h"

void ObjectManager::startLoadModel(DescriptorPool& pool)
{
    /*TransformComponent helmetTransform{};
    helmetTransform.rotation = { 3 * pi<float> / 2, pi<float>, 0 };
    helmetTransform.translation = { 8, -0.5, 9 };
    helmetTransform.scale = { 0.5f, 0.5f, 0.5f };
    loadObjectAsync(device, "model/2.0/damagedhelmet/gltf/damagedhelmet.gltf", helmetTransform);*/

    TransformComponent vikingRoomTransform{};
    vikingRoomTransform.rotation = { pi<float> / 2, pi<float>, 0 };
    vikingRoomTransform.translation = { 7, 0, 7 };
    loadObjectAsyncObj(device, "model/viking_room.obj", "textures/viking_room.png", vikingRoomTransform);

    std::shared_ptr<Model> plane = createPlane(device, 10, 10, { 0, 0, 0 });
    auto plane1 = GameObject::createGameObject(device);
    plane1.setModel(plane);
    plane1.transform.translation.y = 0.1f;
    plane1.createDescriptorSet(pool);
    pushSyncGameObject(std::move(plane1));

    std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
    auto plane2 = GameObject::createGameObject(device);
    plane2.setModel(planeModel);
    plane2.transform.rotation.z = -pi<float> / 2;
    plane2.transform.translation.x = 10.f;
    plane2.transform.translation.y = 1.f;
    plane2.createDescriptorSet(pool);
    pushSyncGameObject(std::move(plane2));

    auto plane3 = GameObject::createGameObject(device);
    plane3.setModel(planeModel);
    plane3.transform.rotation.x = pi<float> / 2;
    plane3.transform.translation.z = 10.f;
    plane3.transform.translation.y = 1.f;
    plane3.createDescriptorSet(pool);
    pushSyncGameObject(std::move(plane3));

    auto spotLight1 = GameObject::makeCamera(device, glm::radians(50.f), 1.f); 
    spotLight1.transform.translation = { -4.0f, -1.0f, 5.5f }; 
    spotLight1.transform.rotation.y = pi<float> *2 / 5; 
    spotLight1.transform.color = { 1.0, 1.0, 1.0, .7 }; 

    auto spotLight2 = GameObject::makeCamera(device, glm::radians(50.f), 1.f); 
    spotLight2.transform.translation = { 1.0f, -2.0f, 2.5f }; 
    spotLight2.transform.rotation.y = pi<float> *2 / 5; 
    spotLight2.transform.color = { 0.0, 1.0, 0.0, .7 }; 
     
    spotLights->emplace(spotLight1.getId(), std::move(spotLight1)); 
    spotLights->emplace(spotLight2.getId(), std::move(spotLight2));  
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
            gameObject.setName(object.name);
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
        return futureObject{ model, transform, GLTF_MODEL, filePath };
        })
    );
}

void ObjectManager::loadObjectAsyncObj(Device& device, const std::string& filePath, const char* filePathTexture, TransformComponent transform)
{
    futureGameObjects.push_back(std::async(std::launch::async, [filePath, filePathTexture, transform, &device]() { 
        std::shared_ptr<Model> model = Model::createModelFromFile(device, filePath, filePathTexture);
        return futureObject{ model, transform, OBJ_MODEL, filePath }; 
        })
    );
}
