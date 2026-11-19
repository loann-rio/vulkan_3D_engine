#include "objectManager.h"

#include "../model/preBuild.h"
#include "../model/ModelBuilder.h"
#include "../model/ModelAsset.h"

#include "../Textures/TextureObject.h"
#include "../Textures/TextureBuilder.h"

#include "../base/Swap_chain.h"

#include "../Scene/SceneSaver.h"

#include <chrono>
#include <utility>
void ObjectManager::startLoadModel()
{

    if (true)
    {
        TextureBuilder textureBuilder(device);
        auto texture = assetManager.textures().create(textureBuilder.fromFile("assets/skybox/cubemap_space.ktx").asCubemap());
        
        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID modelId = assetManager.models().create(builder.fromFile("assets/model/cube.obj").withTexture(texture), currentScene);

        if (modelId)
        {
            createDescriptorSet(assetManager.models().get(modelId), currentScene);

            auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
            gameObject->setName("cubemap");
            gameObject->setModelType(ModelType::OBJ_MODEL);
            gameObject->setModelSubType(ModelSubType::SKYBOX);
            gameObject->setModel(modelId);
            gameObject->saveable = false;
            gameObject->show = false;
            assetManager.gameObject().add(std::move(gameObject));
        }
    }

    if (true)
    {
        std::vector<ModelInstance> instances;
        for (int x = 0; x < 64; x++) {
            for (int z = 0; z < 64; z++) {
                ModelInstance instance;
                instance.position = { x / 5.f - 15.f, 0.0f, z / 5.f - 15.f, 1.f };
                instance.rotation = { 0.0f, static_cast<float>(rand() % 360), 0.0f, 0.0f };
                instance.scale = { 1.f, 1.f, 1.f, 1.f };
                instances.push_back(instance);
            }
        }

        auto texture = assetManager.textures().create(TextureBuilder(device).fromFile("assets/textures/whiteTexture.jpg"));
        auto textureGrass = assetManager.textures().create(TextureBuilder(device).fromFile("assets/textures/GrassBillboard.png"));

        ModelManager::ModelID grassLod1 = assetManager.models().create(ModelBuilder(device, assetManager).fromFile("assets/model/grassLOD/grassLod1.obj").withShadow(false).withTexture(texture), currentScene);
        ModelManager::ModelID grassLod2 = assetManager.models().create(ModelBuilder(device, assetManager).fromFile("assets/model/grassLOD/grassLod2.obj").withShadow(false).withTexture(texture), currentScene);
        ModelManager::ModelID grassLod3 = assetManager.models().create(ModelBuilder(device, assetManager).fromFile("assets/model/grassLOD/grassLod3.obj").withShadow(false).withTexture(texture), currentScene);
        ModelManager::ModelID grassLod4 = assetManager.models().create(ModelBuilder(device, assetManager).fromFile("assets/model/grassLOD/grassLod4.obj").withShadow(false).withTexture(textureGrass), currentScene);

        createDescriptorSet(assetManager.models().get(grassLod1), currentScene);
        createDescriptorSet(assetManager.models().get(grassLod2), currentScene);
        createDescriptorSet(assetManager.models().get(grassLod3), currentScene);
        createDescriptorSet(assetManager.models().get(grassLod4), currentScene);


        auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
        gameObject->setName("grass");
        gameObject->setModelType(ModelType::OBJ_MODEL);
        gameObject->setModel({ grassLod1, grassLod2, grassLod3, grassLod4 });
        gameObject->saveable = false;
        gameObject->setMultipleInstances(instances);
        gameObject->transform.translation = { 12.f, 0.f, 12.f };
		gameObject->createInstanceComputeDescriptorSets(*globalPool);
        assetManager.gameObject().add(std::move(gameObject));


        ComputeObject computeGrass{
            true,
            (instances.size() + 255) / 255, 
            1, 
            1,
            instances.size(),
            dynamic_cast<GameObjectModel*>(assetManager.gameObject().findByName("grass"))
        };

        computeList.push_back(computeGrass);
    }

    {
        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID id = assetManager.models(currentScene).create(builder.fromFile("assets/model/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf"), currentScene);

        if (id)
        {
            createDescriptorSet(assetManager.models().get(id), currentScene);

            auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
            gameObject->setName("testModelBuilder");
            gameObject->setModelType(ModelType::GLTF_MODEL);
            gameObject->setModel(id);
            gameObject->transform.rotation.x = 3.141592f;
            gameObject->transform.rotation.y = 15;
            gameObject->transform.translation = { 0, 0.2f, 8 };
            gameObject->saveable = false;
            gameObject->createPoseResources(*globalPool);

            assetManager.gameObject().add(std::move(gameObject));

        }
    }

    {
        ModelBuilder builder(device, assetManager);
        ModelManager::ModelID id = assetManager.models().create(builder.fromFile("assets\\model\\people\\UAL1_Standard.glb"), currentScene);

        if (id)
        {
            createDescriptorSet(assetManager.models().get(id), currentScene);

            //auto behavior = GameObjectBehavior::createBehaviorFromType("PlayerBehavior", device);

            auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
            gameObject->setName("player");
            //gameObject->setAttachedClass(std::move(behavior));
            gameObject->setModelType(ModelType::GLTF_MODEL);
            gameObject->setModel(id);
            gameObject->transform.rotation = { 0.33f, 0.f, 3.141592f };
            gameObject->transform.translation = { 0.33f, 2.6f, -4.2f };
            gameObject->saveable = false;
            gameObject->createPoseResources(*globalPool);

            auto& obj = assetManager.gameObject(currentScene).add(std::move(gameObject));

            // player camera -> child of player

            auto cameraObject = GameObjectFactory::createGameObject<GameObjectCamera>(device, assetManager, 0.5f, 1.f, 0.1f, 100.f);
            cameraObject->setName("player camera");
            cameraObject->transform.translation = { 1, -1, 1 };
            cameraObject->saveable = false;
            cameraObject->setParent(&obj);
            assetManager.gameObject(currentScene).add(std::move(cameraObject));
        }
    }

    /* {
        auto behavior = GameObjectBehavior::createBehaviorFromType("ChunkManager", device);
		auto gameObject = GameObjectFactory::createGameObject<GameObject>(device);
		gameObject->setName("terrainGenerator");
		gameObject->setAttachedClass(std::move(behavior));
		gameObject->saveable = false;
		pushGameObject(std::move(gameObject));
    }*/

}

void ObjectManager::createPrimitive(PrimitivesModelType type, int detail, TransformComponent transform, const std::string& name, const std::string& filePathTexture, const std::string& targetScene)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
    gameObject->transform = transform; 
    gameObject->setName(name.empty() ? "primitive_" + std::to_string(gameObject->getId()) : name);
	gameObject->setModelType(ModelType::OBJ_MODEL);
    gameObject->setPrimitivesModelType(type);
	gameObject->primitiveLOD = detail;
	gameObject->texturePath = filePathTexture;

    GameObject::id_t id = gameObject->getId();

    //pushGameObject(std::move(gameObject));
    assetManager.gameObject(targetScene).add(std::move(gameObject));
    pushFuture(std::async(std::launch::async, [this, id, type, detail, filePathTexture, targetScene]() mutable {
        std::string scene = targetScene;
        
        ModelManager::ModelID modelID = 0;

        switch (type) {
        case PrimitivesModelType::PLANE:
            modelID = PrebuiltModel::createPlane(
                this->device,
                this->assetManager,
                scene,
                detail,
                1,
                { 1, 1, 1 },
                filePathTexture.empty() ? "assets/textures/whiteTexture.jpg" : filePathTexture,
                20
            );
            break;
        case PrimitivesModelType::CUBE:
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

        return std::vector<futureObject>{
            futureObject{
                modelID ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL,
                id,
                {},
                true,
                modelID,
                targetScene
            }
        };
     }));
}

    /////// scene management ///////
/// <summary>
/// save current scene and load new one
/// </summary>
/// <param name="name"> new scene name </param>
void ObjectManager::switchScene(std::string sceneName)
{
	if (assetManager.scenes.find(sceneName) == assetManager.scenes.end())
		assetManager.createNewScene(sceneName);

	assetManager.switchScene(sceneName);
}

void ObjectManager::loadScene(std::string name)
{
	sceneLoader.load(name);
}

void ObjectManager::saveFullScene()
{
    SceneSaver::save(currentScene, assetManager.gameObject(currentScene));
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
void ObjectManager::updateObjectManager()
{

    // load futures
    auto it = futureGameObjectslist.begin();
    while (it != futureGameObjectslist.end()) { // iter over futures
        if (it->wait_for(std::chrono::seconds(0)) != std::future_status::ready) { // check if future is ready
            ++it;
            continue;
        }
            
        // iter over models in future
        for (futureObject object : it->get()) { 
            if (object.type == ModelType::UNDEFINED_MODEL) 
                continue;

            // get gameobject
            auto* gameObject = dynamic_cast<GameObjectModel*>(assetManager.gameObject(object.targetScene).find(object.id));

            if (!gameObject || !object.modelID)
                continue;

            if (object.instances.size() > 0)
                gameObject->setMultipleInstances(object.instances);
            
            createDescriptorSet(assetManager.models(object.targetScene).get(object.modelID), currentScene);

            gameObject->setModel(object.modelID);
            gameObject->setModelType(object.type);
            gameObject->createPoseResources(*globalPool);
            gameObject->show = true;
        }

        it = futureGameObjectslist.erase(it); // remove from futures
    }

    // switch scene if requested
   
    if (newSceneName.size()) {
        switchScene(newSceneName);
        currentScene = newSceneName;
        newSceneName = "";
    }
}

void ObjectManager::pushGameObject(std::unique_ptr<GameObject> gameObject, const std::string& targetScene)
{
    GameObject::id_t id = gameObject->getId();
    std::string name = gameObject->getName();
    std::type_index type = typeid(*gameObject);

    gameObject->setup();

    assetManager.gameObject(targetScene).add(std::move(gameObject));
}

void ObjectManager::loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, TransformComponent transform, const std::string& name, const std::string& targetScene)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assetManager);
    gameObject->transform = transform; 
    gameObject->setName(name.empty() ? filePath : name);
    gameObject->modelPath = filePath;
    GameObject::id_t id = gameObject->getId(); 

    pushGameObject(std::move(gameObject), targetScene);

    pushFuture(std::async(std::launch::async, [filePath, &device, &assets, id, targetScene]() {
        ModelBuilder builder(device, assets);
        ModelManager::ModelID modelId = assets.models().create(builder.fromFile(filePath), targetScene);

        return std::vector<futureObject>{ futureObject{ modelId ? ModelType::GLTF_MODEL : ModelType::UNDEFINED_MODEL, id, { }, true, modelId, targetScene } };
        }));
}

void ObjectManager::loadObjectAsync(Device& device, AssetManager& assets, const std::string& filePath, const std::string filePathTexture, TransformComponent transform, const std::string& name, const std::string& targetScene)
{
    auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assets);
    gameObject->transform = transform;  
    gameObject->setName(name.empty() ? filePath : name); 

    gameObject->modelPath = filePath;
    gameObject->texturePath = filePathTexture;
    
    GameObject::id_t id = gameObject->getId();

    pushGameObject(std::move(gameObject), targetScene); 

    pushFuture( std::async(std::launch::async, [filePath, filePathTexture, &device, &assets, id, targetScene]()
        {
            TextureBuilder textureBuilder(device);
            auto texture = assets.textures().create(textureBuilder.fromFile(filePathTexture));

            ModelBuilder builder(device, assets);
            ModelManager::ModelID modelId = assets.models().create(builder.fromFile(filePath).withTexture(texture), targetScene);

            return std::vector<futureObject> {futureObject{ modelId ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id, {}, true, modelId, targetScene }};
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

    assetManager.createNewScene(currentScene);
    assetManager.switchScene(currentScene);
    loadScene(currentScene);
};

void ObjectManager::createDescriptorSet(ModelAsset* model, const std::string& targetScene)
{

    for (auto& lod : model->lods) {

        if (lod.materialBuffer) {
            auto textureSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_MODEL_TEXTURES)
                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

            VkDescriptorImageInfo firstInfo = assetManager.textures(targetScene).get(lod.textures[0])->getImageInfo();
            std::vector<VkDescriptorImageInfo> texturesInfo(MAX_MODEL_TEXTURES, firstInfo);
            for (size_t i = 0; i < lod.textures.size(); i++)
                texturesInfo[i] = assetManager.textures(targetScene).get(lod.textures[i])->getImageInfo();

            VkDescriptorBufferInfo materialBufferInfo = lod.materialBuffer->descriptorInfo();

            lod.descriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
            for (auto& set : lod.descriptorSet)
            {
                DescriptorWriter(*textureSetLayout, *globalPool)
                    .writeImage(0, texturesInfo.data(), MAX_MODEL_TEXTURES)
                    .writeBuffer(1, &materialBufferInfo)
                    .build(set);
            }
            continue;
        }

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
