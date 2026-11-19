#include "SceneSaver.h"

#include "../assetManager/GameObjectManager.h"
#include "../objects/GameObject.h"
#include "../../external/json.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace
{
    std::filesystem::path makeScenePath(const std::string& sceneName)
    {
        const std::string filename =
            sceneName.size() >= 5 && sceneName.substr(sceneName.size() - 5) == ".json"
            ? sceneName
            : sceneName + ".json";

        return std::filesystem::path("scenes") / filename;
    }

    nlohmann::json serializeTransform(const TransformComponent& transform)
    {
        return {
            { "translation",{ transform.translation.x, transform.translation.y, transform.translation.z } },
            { "rotation",{ transform.rotation.x,    transform.rotation.y,    transform.rotation.z } },
            { "scale",{ transform.scale.x,       transform.scale.y,       transform.scale.z } },
            { "color",{ transform.color.r,       transform.color.g,       transform.color.b, transform.color.a } }
        };
    }

    nlohmann::json serializeObject(const GameObject& object)
    {
        nlohmann::json data;
        data["type"] = static_cast<int>(object.getType());
        data["transform"] = serializeTransform(object.transform);

        switch (object.getType())
        {
        case GameObjectType::MODEL:
        {
            const auto* model = dynamic_cast<const GameObjectModel*>(&object);
            if (!model)
                break;

            data["modelType"] = static_cast<int>(model->getModelType());

            if (!model->modelPath.empty())
                data["modelPath"] = model->modelPath;

            if (!model->texturePath.empty())
                data["texturePath"] = model->texturePath;

            if (model->getPrimitivesModelType() != PrimitivesModelType::NONE)
            {
                data["primitivesModelType"] = static_cast<int>(model->getPrimitivesModelType());
                data["detail"] = model->primitiveLOD;
            }
            break;
        }
        case GameObjectType::CAMERA:
        {
            const auto* camera = dynamic_cast<const GameObjectCamera*>(&object);
            if (camera)
            {
                data["fov"] = camera->getFov();
                data["aspectRatio"] = camera->getAspectRatio();
                data["nearPlane"] = camera->getNearClip();
                data["farPlane"] = camera->getFarClip();
            }
            break;
        }
        case GameObjectType::SPOT_LIGHT:
        {
            const auto* light = dynamic_cast<const GameObjectSpotLight*>(&object);
            if (light)
            {
                data["fov"] = light->getFov();
                data["aspectRatio"] = light->getAspectRatio();
                data["nearPlane"] = light->getNearClip();
                data["farPlane"] = light->getFarClip();
            }
            break;
        }
        default:
            break;
        }

        if (object.hasAttachedClass)
            data["behavior_type"] = object.getAttachedClassType();

        return data;
    }
}

void SceneSaver::save(const std::string& sceneName, const GameObjectManager& objects)
{
    nlohmann::json scene = { { "objects", nlohmann::json::object() } };

    for (const GameObject* object : objects.getByType<GameObject>())
    {
        if (!object || !object->saveable || object->toBeRemoved)
            continue;

        scene["objects"][object->getName()] = serializeObject(*object);
    }

    const std::filesystem::path scenePath = makeScenePath(sceneName);
    std::filesystem::create_directories(scenePath.parent_path());

    std::ofstream output(scenePath);
    if (!output.is_open())
        throw std::runtime_error("Could not write scene file: " + scenePath.string());

    output << scene.dump(4) << '\n';
    if (!output)
        throw std::runtime_error("Failed while writing scene file: " + scenePath.string());
}