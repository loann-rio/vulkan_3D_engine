#pragma once


#include "../../external/json.hpp"

#include <filesystem>
#include <string>
#include <glm/glm.hpp>

class Device;
class AssetManager;
class ObjectManager;
struct TransformComponent;

class SceneLoader final
{
public:
    SceneLoader(Device& device, AssetManager& assetManager, ObjectManager& objectManager)
        : device_(device), assetManager_(assetManager), objectManager_(objectManager)
    {}

    nlohmann::json load(
        const std::string& sceneName
    );

private:
    static std::filesystem::path makeScenePath(
        const std::string& sceneName
    );

    static glm::vec3 readVec3(
        const nlohmann::json& value,
        const char* key,
        const glm::vec3& fallback
    );

    static TransformComponent readTransform(
        const nlohmann::json& data
    );

    void loadModel(
        const std::string& name,
        const nlohmann::json& data,
        const TransformComponent& transform
    );

    void loadCamera(
        const std::string& name,
        const nlohmann::json& data,
        const TransformComponent& transform
    );

    void loadSpotLight(
        const std::string& name,
        const nlohmann::json& data,
        const TransformComponent& transform
    );

    void loadGenericObject(
        const std::string& name,
        const nlohmann::json& data,
        const TransformComponent& transform
    );

    Device& device_;
    AssetManager& assetManager_;
    ObjectManager& objectManager_;
};