#pragma once

#include <algorithm> 
#include <memory>
#include <string>   
#include <vector>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../../assetManager/TextureManager.h"

#include "../Vertex/IVertexData.h"

#include "../BoundingBox.h"

#include "../ModelNode.h"

namespace moDecoder {

    /// <summary>
    /// Return lowercase extension without dot
    /// </summary>
    /// <param name="path">path to texture</param>
    /// <returns>lowercase extension without dot</returns>
    inline std::string getExtension(const std::string& path) {
        auto pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return {};

        std::string ext = path.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return ext;
    }
}



struct DecodedMaterial {
    // References to textures IDs from TextureManager
    std::string albedoTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;

    // Scalar parameters
    float metallic = 0.0f;
    float roughness = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

    std::string name;
};

struct DecodedModel {
    std::unique_ptr<IVertexData> vertices;
    std::vector<uint32_t> indices;
    std::vector<Primitive> primitives;
    std::vector<DecodedMaterial> materials;
    std::vector<std::unique_ptr<Node>> nodes;
    BoundingBox aabb;

    std::string name;
};



struct Material {
    // References to textures IDs from TextureManager
    TextureManager::TextureID albedoTexture = 0;
    TextureManager::TextureID normalTexture = 0;
    TextureManager::TextureID metallicRoughnessTexture = 0;

    // Scalar parameters
    float metallic = 0.0f;
    float roughness = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

    std::vector<VkDescriptorSet> descriptorSet; 

    std::string name;
};





class IModelDecoder {
public:
    virtual ~IModelDecoder() = default;

    virtual bool canDecode(const std::filesystem::path& path) const = 0;
    virtual DecodedModel decode(const std::filesystem::path& path) const = 0;
};
