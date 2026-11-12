#include "SceneLoader.h"
#include "../objects/ObjectManager.h"

#include "../../external/json.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

nlohmann::json SceneLoader::load(const std::string& sceneName)
{
    const std::filesystem::path scenePath = makeScenePath(sceneName);
    nlohmann::json scene;

    std::ifstream input(scenePath);
    if (!input.is_open())
    {
        scene = { { "objects", nlohmann::json::object() } };

        std::ofstream output(scenePath);
        if (!output.is_open())
        {
            throw std::runtime_error("Could not create scene file: " + scenePath.string());
        }

        output << scene.dump(4) << '\n';
    }
    else
    {
        input >> scene;
    }

    if (!scene.is_object() || !scene.contains("objects") || !scene["objects"].is_object())
    {
        throw std::runtime_error("Scene must contain an 'objects' object: " + scenePath.string());
    }

    for (const auto& [objectName, data] : scene["objects"].items())
    {
        if (!data.is_object())
        {
            continue;
        }

        const auto type = data.value("type", static_cast<int>(GameObjectType::UNKNOWN));
        const TransformComponent transform = readTransform(data);

        if (type == static_cast<int>(GameObjectType::MODEL))
        {
            loadModel(objectName, data, transform);
        }
        else if (type == static_cast<int>(GameObjectType::CAMERA))
        {
            loadCamera(objectName, data, transform);
        }
        else if (type == static_cast<int>(GameObjectType::SPOT_LIGHT))
        {
            loadSpotLight(objectName, data, transform);
        }
        else
        {
            loadGenericObject(objectName, data, transform);
        }
    }

    if (assetManager_.gameObject().findByName("mainCamera") == nullptr)
    {
        auto camera = GameObjectFactory::createGameObject<GameObjectCamera>(
            device_, assetManager_, glm::radians(50.0f), 1.0f, 0.1f, 100.0f);
        camera->setName("mainCamera");
        objectManager_.pushGameObject(std::move(camera));
    }

    return scene;
}

std::filesystem::path SceneLoader::makeScenePath(const std::string& sceneName)
{
    const std::string filename =
        sceneName.size() >= 5 && sceneName.substr(sceneName.size() - 5) == ".json"
        ? sceneName : sceneName + ".json";

    return std::filesystem::path("scenes") / filename;
}

glm::vec3 SceneLoader::readVec3(const nlohmann::json& value, const char* key, const glm::vec3& fallback)
{
    if (!value.contains(key) || !value[key].is_array() || value[key].size() < 3)
    {
        return fallback;
    }

    return {
        value[key][0].get<float>(),
        value[key][1].get<float>(),
        value[key][2].get<float>()
    };
}

TransformComponent SceneLoader::readTransform(const nlohmann::json& data)
{
    TransformComponent transform{};

    if (!data.contains("transform") || !data["transform"].is_object())
    {
        return transform;
    }

    const auto& value = data["transform"];
    transform.translation = readVec3(value, "translation", transform.translation);
    transform.rotation = readVec3(value, "rotation", transform.rotation);
    transform.scale = readVec3(value, "scale", transform.scale);

    if (value.contains("color") && value["color"].is_array() && value["color"].size() >= 4)
    {
        transform.color = {
            value["color"][0].get<float>(),
            value["color"][1].get<float>(),
            value["color"][2].get<float>(),
            value["color"][3].get<float>()
        };
    }

    return transform;
}

void SceneLoader::loadModel(const std::string& name, const nlohmann::json& data, const TransformComponent& transform)
{
    if (data.contains("modelPath"))
    {
        const std::string modelPath = data["modelPath"].get<std::string>();
        const bool isGltf = modelPath.find(".gltf") != std::string::npos || modelPath.find(".glb") != std::string::npos;

        if (isGltf)
        {
            objectManager_.loadObjectAsync(device_, assetManager_, modelPath, transform, name);
        }
        else
        {
            const std::string texturePath =
                data.value("texturePath", std::string("assets/textures/whiteTexture.jpg"));
            objectManager_.loadObjectAsync(
                device_, assetManager_, modelPath, texturePath, transform, name);
        }

        return;
    }

    if (data.contains("primitivesModelType"))
    {
        const auto primitiveType = static_cast<PrimitivesModelType>(
            data["primitivesModelType"].get<int>());
        const int detail = data.value("detail", 10);
        const std::string texturePath = data.value("texturePath", std::string{});

        objectManager_.createPrimitive(primitiveType, detail, transform, name, texturePath);
    }

}

void SceneLoader::loadCamera(const std::string& name, const nlohmann::json& data, const TransformComponent& transform)
{
    const float fov = data.value("fov", glm::radians(50.0f));
    const float aspectRatio = data.value("aspectRatio", 1.0f);
    const float nearPlane = data.value("nearPlane", 0.1f);
    const float farPlane = data.value("farPlane", 100.0f);

    auto camera = GameObjectFactory::createGameObject<GameObjectCamera>(
        device_, assetManager_, fov, aspectRatio, nearPlane, farPlane);
    camera->transform = transform;
    camera->setName(name);
    objectManager_.pushGameObject(std::move(camera));
}

void SceneLoader::loadSpotLight(const std::string& name, const nlohmann::json& data, const TransformComponent& transform)
{
    const float fov = data.value("fov", 1.0f);
    const float aspectRatio = data.value("aspectRatio", 1.0f);
    const float nearPlane = data.value("nearPlane", 0.1f);
    const float farPlane = data.value("farPlane", 100.0f);

    auto light = GameObjectFactory::createGameObject<GameObjectSpotLight>(
        device_, assetManager_, fov, aspectRatio, nearPlane, farPlane);
    light->transform = transform;
    light->setName(name);
    objectManager_.pushGameObject(std::move(light));
}

void SceneLoader::loadGenericObject(const std::string& name, const nlohmann::json& data, const TransformComponent& transform) 
{
    auto object = GameObjectFactory::createGameObject<GameObject>(device_, assetManager_);
    object->transform = transform;
    object->setName(name);

    if (data.contains("behavior_type"))
    {
        const std::string behaviorType = data["behavior_type"].get<std::string>();
        auto behavior = GameObjectBehavior::createBehaviorFromType(behaviorType, device_);
        if (behavior)
        {
            object->setAttachedClass(std::move(behavior));
        }
    }

    objectManager_.pushGameObject(std::move(object));
}

