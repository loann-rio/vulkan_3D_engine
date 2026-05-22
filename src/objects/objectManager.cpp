#include "ObjectManager.h"

#include "../model/preBuild.h"

#include "../Textures/TextureObject.h"
#include "../Textures/TextureBuilder.h"

#include "../model/ModelBuilder.h"


#include <chrono>
#include <fstream>


void ObjectManager::startLoadModel()
{
    for (int i = 0; i < 1; i++)
    {
        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID id = assetManager.models().create(builder.fromFile("model/buster_drone/scene.gltf"));

        if (!id) continue;

        createDescriptorSet(assetManager.models().get(id));

        auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
        gameObject->setName("testModelBuilder");
        gameObject->setModelType(ModelType::OBJ_MODEL);
        gameObject->setModel(id);
        gameObject->transform.rotation.x = 3.141592f;
        gameObject->transform.rotation.y = i * 15;
        gameObject->transform.translation = { i, 0.2f, 8 };
        gameObject->saveable = false;
        gameObject->createDescriptorSet(*globalPool);
        pushGameObject(std::move(gameObject));
    }

    if (true)
    {
        TextureBuilder textureBuilder(device);
        auto texture = assetManager.textures().create(textureBuilder.fromFile("skybox/cubemap_space.ktx").asCubemap());

        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID modelId = assetManager.models().create(builder.fromFile("model/cube.obj").withTexture(texture));

        createDescriptorSet(assetManager.models().get(modelId));
        

        auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
        gameObject->setName("cubemap1");
        gameObject->setModelType(ModelType::OBJ_MODEL);
        gameObject->setModelSubType(ModelSubType::SKYBOX);
        gameObject->setModel(modelId);
        gameObject->saveable = false;
        gameObject->show = false;
        pushGameObject(std::move(gameObject));
    }

    /* {
        auto behavior = GameObjectBehavior::createBehaviorFromType("ChunkManager", device);
		auto gameObject = GameObjectFactory::createGameObject<GameObject>(device);
		gameObject->setName("terrainGenerator");
		gameObject->setAttachedClass(std::move(behavior));
		gameObject->saveable = false;
		pushGameObject(std::move(gameObject));
    }*/
 
    {

		std::vector<Model::Instance> instances;
		for (int x = 0; x < 150; x++) {
			for (int z = 0; z < 150; z++) {
				Model::Instance instance;
                instance.position = { x / 5.f - 15.f, 0.0f, z / 5.f - 15.f };
				instance.rotation = { 0.0f, static_cast<float>(rand() % 360), 0.0f };
                instance.scale = { 1, 1, 1 };
				instances.push_back(instance);
			}
		}

        std::shared_ptr<Model> cube = Model::createModelFromFile(device, assetManager, std::vector<std::array<std::string, 2>>{ {"model/grassLOD/grassLod4.obj", "textures/GrassBillboard.png"} , {"model/grassLOD/grassLod1.obj", "textures/whiteTexture.jpg"}, {"model/grassLOD/grassLod2.obj", ""} , {"model/grassLOD/grassLod3.obj", ""} });
        cube->computeShadow = false;

        /*TextureBuilder textureBuilder(device);
        auto texture = assetManager.textures().create(textureBuilder.fromFile("textures/whiteTexture.jpg"));

        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID modelId = assetManager.models().create(builder.fromFile("model/grassLOD/grassLod1.obj").withTexture(texture));*/

        auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
        gameObject->setName("testlod");
        gameObject->setModelType(ModelType::OBJ_MODEL);
        gameObject->setModel(cube);
        gameObject->saveable = false;
		gameObject->setMultipleInstances(instances);
        gameObject->createDescriptorSet(*globalPool);
        pushGameObject(std::move(gameObject));
    }
    

}

void ObjectManager::createPrimitive(PrimitivesModelType type, int detail, TransformComponent transform, const std::string& name, const std::string& filePathTexture)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
    gameObject->transform = transform; 
    gameObject->setName(name.empty() ? "primitive_" + std::to_string(gameObject->getId()) : name);
	gameObject->setModelType(ModelType::OBJ_MODEL);
    gameObject->setPrimitivesModelType(type);
	gameObject->primitiveLOD = detail;
	gameObject->texturePath = filePathTexture;

    GameObject::id_t id = gameObject->getId();

    pushGameObject(std::move(gameObject));

    pushFuture(std::async(std::launch::async, [this, id, type, detail, filePathTexture]() {
        std::shared_ptr<Model> primitive;

        switch (type) {
        case PrimitivesModelType::PLANE:
            primitive = PrebuiltModel::createPlane(this->device, this->assetManager, detail, 1, { 0, 0, 0 }, filePathTexture.empty() ? "textures/whiteTexture.jpg" : filePathTexture, 20);
            break;
        case PrimitivesModelType::CUBE:
            primitive = PrebuiltModel::createCube(this->device, this->assetManager);
            break;
        case PrimitivesModelType::SPHERE:
            break;
        case PrimitivesModelType::CYLINDER:
            break;
        case PrimitivesModelType::CONE:
            break;
        default:
            std::cerr << "Unknown primitive type\n";
            break;
        }

        return std::vector<futureObject>{ futureObject{ primitive, primitive ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id } };
        })
     );
}

    /////// get and remove ///////

GameObject* ObjectManager::get(const GameObject::id_t id)
{
    auto it = gameObjects->find(id);
    return (it != gameObjects->end()) ? it->second.get() : nullptr;
}

GameObject* ObjectManager::get(const std::string& name)
{
    auto it = gameObjectsByName.find(name);
    return (it != gameObjectsByName.end()) ? it->second : nullptr;
}

void ObjectManager::removeGameObject(const GameObject::id_t id)
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

void ObjectManager::removeGameObject(const std::string& name)
{
	removeGameObject(get(name)->getId());
}

void ObjectManager::removeGameObject(GameObject* gameObject)
{
	removeGameObject(gameObject->getId());
}

    /////// scene management ///////
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

	// keep skybox
    get("cubemap")->toBeRemoved = false;

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
        std::unique_ptr<GameObjectBehavior> behavior;

        if (element.value().contains("transform")) {
            auto t = element.value()["transform"];
            transform.translation = glm::vec3{ t["translation"][0], t["translation"][1], t["translation"][2] };
            transform.rotation = glm::vec3{ t["rotation"][0], t["rotation"][1], t["rotation"][2] };
            transform.scale = glm::vec3{ t["scale"][0], t["scale"][1], t["scale"][2] };
            transform.color = glm::vec4{ t["color"][0], t["color"][1], t["color"][2], t["color"][3] };
        }

        if (element.value().contains("behavior_type")) {
            std::string classType = element.value()["behavior_type"];
            behavior = GameObjectBehavior::createBehaviorFromType(classType, device);       
        }

        if (objType == static_cast<int>(GameObjectType::MODEL)) {

			// two main types of model: primitives and loaded from file
			// loaded from file: gltf/glb or obj + texture
			// primitives: plane, cube, sphere, cylinder, cone
			// default texture for obj is whiteTexture.jpg
            
            if (element.value().contains("modelPath")) 
            {
                std::string modelPath = element.value()["modelPath"];

                if (modelPath.find(".gltf") != std::string::npos || modelPath.find(".glb") != std::string::npos)
                    loadObjectAsync(device, assetManager, modelPath, transform, objName);
                else 
                {
                    std::string texturePath = element.value().contains("texturePath") ? element.value()["texturePath"] : "textures/whiteTexture.jpg";
                    loadObjectAsync(device, assetManager, modelPath, texturePath, transform, objName);
                }
            }
            else if(element.value().contains("primitivesModelType"))
            {
                PrimitivesModelType primitiveType = static_cast<PrimitivesModelType>(element.value()["primitivesModelType"]);
                std::string texturePath = element.value()["texturePath"];
                createPrimitive(primitiveType, 10, transform, objName, texturePath);
            }
        }

        else if (objType == static_cast<int>(GameObjectType::CAMERA)) {
            float fov         = element.value()["fov"];
            float aspectRatio = element.value()["aspectRatio"];
            float nearPlane   = element.value()["nearPlane"];
            float farPlane    = element.value()["farPlane"];
            
            auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, assetManager, fov, aspectRatio, nearPlane, farPlane);
            cameraObject->transform = transform;
            cameraObject->setName(objName);
            pushGameObject(std::move(cameraObject));
        }
        else if (objType == static_cast<int>(GameObjectType::SPOT_LIGHT)) {
            float fov = element.value()["fov"];
            float aspectRatio = element.value()["aspectRatio"];
            float nearPlane = element.value()["nearPlane"];
            float farPlane = element.value()["farPlane"];
            auto spotLight = GameObjectFactory::createGameObject<GameObjectSpotLight>(device, assetManager, fov, aspectRatio, nearPlane, farPlane);
            spotLight->transform = transform;
            spotLight->setName(objName);
            pushGameObject(std::move(spotLight));
        }
        else
        {
            auto gameObject = GameObjectFactory::createGameObject<GameObject>(device, assetManager);

            if (behavior) gameObject->setAttachedClass(std::move(behavior));

            gameObject->setName(objName); 
            pushGameObject(std::move(gameObject));

        }
    }

	// ensure there is a main camera
    if (get("mainCamera") == nullptr) { 
        auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, assetManager, glm::radians(50.f), 1.f, .1f, 100.f);
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

    // type-specific params
    if (objJson["type"] == GameObjectType::MODEL) {
        auto* model = dynamic_cast<GameObjectModel*>(gameObject);
		
		objJson["modelType"] = static_cast<int>(model->getModelType());

        if (!model->modelPath.empty()) {
            objJson["modelPath"] = model->modelPath;
        }

        if (model->getModelType() == ModelType::OBJ_MODEL)
        {
            if (model->getPrimitivesModelType() != PrimitivesModelType::NONE) {
                objJson["primitivesModelType"] = static_cast<int>(model->getPrimitivesModelType());
                objJson["detail"] = model->primitiveLOD;
			}

            objJson["texturePath"] = model->texturePath;
        }
        
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
    if (gameObject->hasAttachedClass) {

		std::cout << "Saving attached class: " << gameObject->getAttachedClassType() << " for object " << gameObject->getName() << std::endl;
        objJson["behavior_type"] = gameObject->getAttachedClassType();
    }

    currentSceneJson["objects"][gameObject->getName()] = objJson;

}

void ObjectManager::createDescriptorSet(ModelAsset* model)
{

    for (auto& lod : model->lods) {
        for (auto& material : lod.materials)
        {
            auto textureSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

            material.descriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT * lod.materials.size());
            for (int i = 0; i < material.descriptorSet.size(); i++)
            {
                auto imageInfo = assetManager.textures().get(material.albedoTexture)->getImageInfo();
                DescriptorWriter(*textureSetLayout, *globalPool)
                    .writeImage(0, &imageInfo)
                    .build(material.descriptorSet[i]);
            }
        }
    }  
}

void ObjectManager::updateGameObject(float frameTime)
{
    std::vector<GameObject::id_t> toRemove;

    for (auto& [id, obj] : *gameObjects) {
        if (!obj->toBeRemoved) {
            obj->loop(this);
            continue;
        }

        // Handle removal
        if (obj->getType() == GameObjectType::MODEL) {
            auto* modelObj = dynamic_cast<GameObjectModel*>(obj.get());
            if (modelObj->show) {
                modelObj->show = false; // Hide first to avoid descriptor issues
                continue;
            }
        }

        // Either not a model, or already hidden — safe to remove
        toRemove.push_back(id);
    }

    // Remove after iteration
    for (auto id : toRemove) {
        removeGameObject(id);
    }

    // update GLTF game objects
    {
        std::vector<GameObjectModel*> objects = getByType<GameObjectModel>();
        for (auto obj : objects) {
            obj->update(frameTime);
        }
    }

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
    currentSceneJson = json::parse(R"({})");
    
    for (auto& [id, obj] : *gameObjects) {
        if (obj->saveable)
            addObjectToScene(obj.get());
	}

    // save back to file
    std::ofstream o(scenePath);
    o << currentSceneJson << std::endl;
}
    
    /////// add objects ///////

void ObjectManager::pushFuture(std::future<std::vector<futureObject>> futures)
{
    futureGameObjectslist.push_back(std::move(futures));
}

/// <summary>
/// take the loaded model from the future and put it in a game object
/// </summary>
/// <param name="pool">global model pool</param>
void ObjectManager::pushModel()
{
    // load futures
    auto it = futureGameObjectslist.begin();
    while (it != futureGameObjectslist.end()) { // iter over futures
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // check if future is ready
            
            // iter over models in future
            for (futureObject object : it->get()) { 

                // confirm that its a model
                if (object.type != ModelType::UNDEFINED_MODEL) {

                    // get gameobject
                    auto* gameObject = dynamic_cast<GameObjectModel*>(get(object.id));

                    if (!gameObject) {
                        it = futureGameObjectslist.erase(it);
                        continue;
                    }

                    if (object.instances.size() > 0) {
                        gameObject->setMultipleInstances(object.instances);
                    }

                    gameObject->setModel(object.model);
                    gameObject->setModelType(object.type);
                    gameObject->createDescriptorSet(*globalPool);
                    gameObject->show = true;
                }
            }

            it = futureGameObjectslist.erase(it); // remove from futures
        }
        else {
            ++it;
        }
    }
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

void ObjectManager::loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
    gameObject->transform = transform; 
    gameObject->setName(name.empty() ? filePath : name);
    gameObject->modelPath = filePath;
    GameObject::id_t id = gameObject->getId(); 

    pushGameObject(std::move(gameObject));

    pushFuture( std::async(std::launch::async, [filePath, &device, &assets, id]() {
        std::shared_ptr<GlTFModel::ModelGltf> model = GlTFModel::createModelFromFile(device, assets, filePath);
        return std::vector<futureObject>{ futureObject{ model, model ? ModelType::GLTF_MODEL : ModelType::UNDEFINED_MODEL, id }};
        }) 
	);
}

void ObjectManager::loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, const std::string filePathTexture, TransformComponent transform, const std::string& name)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
    gameObject->transform = transform;  
    gameObject->setName(name.empty() ? filePath : name); 

    gameObject->modelPath = filePath;
    gameObject->texturePath = filePathTexture;
    
    GameObject::id_t id = gameObject->getId();

    pushGameObject(std::move(gameObject)); 
     
    pushFuture( std::async(std::launch::async, [filePath, filePathTexture, &device, &assets, id]() {
        std::shared_ptr<Model> model = Model::createModelFromFile(device, assets, filePath, filePathTexture.c_str());
        return std::vector<futureObject> {futureObject{ model, model ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id }};
        }) 
     );
}

ObjectManager::ObjectManager(Device& device, AssetManager& assetManager) : device{ device }, assetManager{ assetManager }
{
    globalPool = DescriptorPool::Builder(device)
        .setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 320)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 640)
        .build();

    gameObjects = std::make_shared<GameObject::Map>();
    loadScene(currentScene);
};

