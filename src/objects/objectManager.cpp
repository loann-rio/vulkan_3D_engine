#include "ObjectManager.h"

#include "../model/preBuild.h"

#include "../base/VoronoiNoise.h"

#include "../GameObjectClassAssets/TerrainGenerator.h"
#include "../GameObjectClassAssets/mainLightBehavior.h"

#include <random>
#include <chrono>
#include <stdlib.h>

//REGISTER_BEHAVIOR(TerrainGenerator);

void ObjectManager::startLoadModel(DescriptorPool& pool)
{

    /*auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f , .1f, 100.f);
    cameraObject->transform.translation = { -2.f, 1.1f, 1.5f };
    cameraObject->transform.rotation = { -0.327, 1.475, 0 };
    cameraObject->setName("mainCamera");
    pushGameObject(std::move(cameraObject)); */

    /*auto spotLight1 = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, 1.5, 1.f, .1f, 100.f);
    spotLight1->transform.translation = { -5.f, -4.0f, -7.6f };
    spotLight1->transform.rotation = { -.07, 0.67, -0.358 };
    spotLight1->transform.color = { 1.0, 1.0, 1.0, .7 };
    spotLight1->setName("spotLight1");
    pushGameObject(std::move(spotLight1));*/

    /*auto terrainManager = GameObjectFactory::createGameObject<GameObject>(device);
    terrainManager->setAttachedClass(std::make_unique<TerrainGenerator>(device));
    terrainManager->setName("terrain G");

    

    pushGameObject(std::move(terrainManager));*/

    /*TransformComponent vikingRoomTransform{};
    vikingRoomTransform.rotation = { 0, 0, 0 };
    vikingRoomTransform.translation = { 7, 0, 7 };
    
	loadObjectAsync(device, "model/DamagedHelmet.gltf", vikingRoomTransform, "damage_helmet");

    TransformComponent vikingRoomTransform2{};
    vikingRoomTransform2.rotation = { 0, 0, 0 };
    vikingRoomTransform2.translation = { 7, 0, 5 };
    loadObjectAsyncObj(device, "model/coloredTree1.obj", "textures/whiteTexture.jpg", vikingRoomTransform2, "viking");*/

    /*std::shared_ptr<Model> plane = createPlane(device, 10, 10, { 0, 0, 0 });
    auto plane1 = GameObjectFactory::createGameObject<GameObjectModel>(device);
    plane1->setModel(plane);
    plane1->transform.translation.y = 0.1f;
    plane1->createDescriptorSet(pool);
    pushGameObject(std::move(plane1));*/

 //   std::shared_ptr<Model> planeModel = createPlane(device, 2, 10, { 0, 0, 0 }, "textures/emptyTexture.jpg");
 //   auto plane2 = GameObjectFactory::createGameObject<GameObjectModel>(device);
 //   plane2->setModel(planeModel);
 //   plane2->transform.rotation.z = -pi<float> / 2;
 //   plane2->transform.translation.x = 10.f;
 //   plane2->transform.translation.y = 1.f;
 //   plane2->createDescriptorSet(pool);
 //   plane2->setName("plane");
 //   pushGameObject(std::move(plane2));

 //   auto plane3 = GameObjectFactory::createGameObject<GameObjectModel>(device);
 //   plane3->setModel(planeModel);
 //   plane3->transform.rotation.x = pi<float> / 2;
 //   plane3->transform.translation.z = 10.f;
 //   plane3->transform.translation.y = 1.f;
 //   plane3->createDescriptorSet(pool);
 //   pushGameObject(std::move(plane3));


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
    gameObject->modelPath = filePath;
    GameObject::id_t id = gameObject->getId(); 

    pushGameObject(std::move(gameObject));

    futureGameObjects.push_back(std::async(std::launch::async, [filePath, &device, id]() {
        std::shared_ptr<GlTFModel::ModelGltf> model = GlTFModel::createModelFromFile(device, filePath);
        return futureObject{ model, model ? GLTF_MODEL : UNDEFINED_MODEL, id };
        }) 
    );
}

void ObjectManager::loadObjectAsyncObj(Device& device, const std::string& filePath, const std::string filePathTexture, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device); 
    gameObject->transform = transform;  
    gameObject->setName(name.empty() ? filePath : name); 

    gameObject->modelPath = filePath;
    gameObject->texturePath = filePathTexture;
    
    GameObject::id_t id = gameObject->getId();

    pushGameObject(std::move(gameObject)); 
     
    futureGameObjects.push_back(std::async(std::launch::async, [filePath, filePathTexture, &device, id]() { 
        std::shared_ptr<Model> model = Model::createModelFromFile(device, filePath, filePathTexture.c_str());
        return futureObject{ model, model ? OBJ_MODEL : UNDEFINED_MODEL, id };
        })
    );
}

/// <summary>
/// save current scene and load new one
/// </summary>
/// <param name="name"> new scene name </param>
void ObjectManager::switchScene(std::string name)
{
    // save current scene
    saveFullScene();

	// mark all objects to be removed
    for (auto obj = gameObjects->begin(); obj != gameObjects->end(); obj++) {
        obj->second->toBeRemoved = true;
	}

	// keep main camera
    get("mainCamera")->toBeRemoved = false;

	// clear all loaded async models
    futureGameObjects.clear();
    futureGameObjectslist.clear();

    // load new scene
    currentScene = name;
    scenePath = "scenes/" + name + ".json";
    loadScene(name);

}

void ObjectManager::loadScene(std::string name)
{
	// test if file exists
    scenePath = "scenes/" + name + ".json";

	std::ifstream i(scenePath);
    if (!i.good()) {
        createScene(name);
        i.open(scenePath);
	}

    i >> currentSceneJson;

    for (auto& element : currentSceneJson["objects"].items()) {

        std::string objName = element.key();
        int objType = element.value()["type"];
        

        TransformComponent transform{};

        if (element.value().contains("transform")) {
            auto t = element.value()["transform"];
            transform.translation = glm::vec3{ t["translation"][0], t["translation"][1], t["translation"][2] };
            transform.rotation = glm::vec3{ t["rotation"][0], t["rotation"][1], t["rotation"][2] };
            transform.scale = glm::vec3{ t["scale"][0], t["scale"][1], t["scale"][2] };
            transform.color = glm::vec4{ t["color"][0], t["color"][1], t["color"][2], t["color"][3] };
        }

        //if (element.value().contains("attachedClass")) {
        //    std::string classType = element.value()["attachedClass"]["type"];
            //json classData = element.value()["attachedClass"]["data"];
            //auto behavior = GameObjectBehavior::createFromType(classType, device);
            //gameObject->setAttachedClass(std::move(behavior));
        //}


        if (objType == static_cast<int>(GameObjectType::MODEL)) {
            std::string modelPath = element.value()["modelPath"];
            std::string texturePath = element.value()["texturePath"];

            if (modelPath.find(".gltf") != std::string::npos || modelPath.find(".glb") != std::string::npos)
                loadObjectAsync(device, modelPath, transform, objName);
			else
                loadObjectAsyncObj(device, modelPath, texturePath, transform, objName);
        }

        else if (objType == static_cast<int>(GameObjectType::CAMERA)) {
            float fov         = element.value()["fov"];
            float aspectRatio = element.value()["aspectRatio"];
            float nearPlane   = element.value()["nearPlane"];
            float farPlane    = element.value()["farPlane"];
            
            auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, fov, aspectRatio, nearPlane, farPlane);
            cameraObject->transform = transform;
            cameraObject->setName(objName);
            pushGameObject(std::move(cameraObject));
        }
        else if (objType == static_cast<int>(GameObjectType::SPOT_LIGHT)) {
            float fov = element.value()["fov"];
            float aspectRatio = element.value()["aspectRatio"];
            float nearPlane = element.value()["nearPlane"];
            float farPlane = element.value()["farPlane"];
            auto spotLight = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, fov, aspectRatio, nearPlane, farPlane);
            spotLight->transform = transform;
            spotLight->setName(objName);
            pushGameObject(std::move(spotLight));
        }
    }

    if (get("mainCamera") == nullptr) {
        auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, glm::radians(50.f), 1.f, .1f, 100.f);
        cameraObject->setName("mainCamera");
        pushGameObject(std::move(cameraObject));
	}

}

void ObjectManager::addObjectToScene(GameObject* gameObject)
{
    
    if (!currentSceneJson.contains("objects")) {
        currentSceneJson["objects"] = json::object();
    }

    // serialize gameObject
	json objJson;
    objJson["type"] = static_cast<int>(gameObject->getType());

    TransformComponent& t = gameObject->transform;
    objJson["transform"]["translation"] = { t.translation.x, t.translation.y, t.translation.z          };
    objJson["transform"]["rotation"]    = { t.rotation.x,    t.rotation.y,    t.rotation.z             };
    objJson["transform"]["scale"]       = { t.scale.x,       t.scale.y,       t.scale.z                };
    objJson["transform"]["color"]       = { t.color.r,       t.color.g,       t.color.b,    t.color.a  };

    // TODO:
	// parent object
	// attached class
	// sub type
	// pre-build params

    // type-specific params
    if (objJson["type"] == GameObjectType::MODEL) {
        auto* model = dynamic_cast<GameObjectModel*>(gameObject);
		
		objJson["modelType"] = static_cast<int>(model->getModelType());
        objJson["modelPath"] = model->modelPath;
        objJson["texturePath"] = model->texturePath;
    }
    else if (objJson["type"] == GameObjectType::CAMERA) {
        auto* cam = dynamic_cast<GameObjectCamera*>(gameObject);
        objJson["fov"] = cam->getFov();
        objJson["aspectRatio"] = cam->getAspectRatio();
        objJson["nearPlane"] = cam->getNearClip();
        objJson["farPlane"] = cam->getFarClip();
    }
    else if (objJson["type"] == GameObjectType::SPOT_LIGHT) {
        auto* light = dynamic_cast<GameObjectSpotLight*>(gameObject);
        objJson["fov"] = light->getFov();
        objJson["aspectRatio"] = light->getAspectRatio();
        objJson["nearPlane"] = light->getNearClip();
        objJson["farPlane"] = light->getFarClip();
    }

    // save attached class
    //if (gameObject->hasAttachedClass) { 
    //    objJson["attachedClass"]["type"] = gameObject->getAttachedClassName();
        //objJson["attachedClass"]["data"] = gameObject->attachedClass->toJson();
    //}
   
    currentSceneJson["objects"][gameObject->getName()] = objJson;

}

void ObjectManager::createScene(std::string name)
{
    json empty = json::parse(R"({})");

	std::string path = "scenes/" + name + ".json";

	// create json file with name
    std::ofstream o(path);
    o << empty << std::endl;
}

void ObjectManager::saveFullScene()
{
    
    for (auto& [id, obj] : *gameObjects) {
        addObjectToScene(obj.get());
	}

    // save back to file
    std::ofstream o(scenePath);
    o << currentSceneJson << std::endl;
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

