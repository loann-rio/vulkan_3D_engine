#pragma once

#include <algorithm> 
#include <memory>
#include <string>   
#include <vector>
#include <array>

#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../../assetManager/TextureManager.h"


#include "../Vertex/IVertexData.h"

#include "../BoundingBox.h"

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

struct Primitive {
    BoundingBox aabb;

    uint32_t firstIndex;
    uint32_t indexCount;

    uint32_t materialIndex = 0;

    uint32_t nodeIndex = -1;
    uint32_t skinIndex = -1;
};

// Texture source: either a file on disk or already decoded RGBA8 pixels (embedded gltf images)
struct DecodedTexture {
    DecodedTexture() = default;
    DecodedTexture(std::string filePath) : path(std::move(filePath)) {}

    std::string path;

    std::vector<unsigned char> pixels;
    uint32_t width = 0;
    uint32_t height = 0;

    bool empty() const { return path.empty() && pixels.empty(); }
};

struct DecodedMaterial {
    DecodedTexture albedoTexture;
    DecodedTexture normalTexture;
    DecodedTexture metallicRoughnessTexture;
    DecodedTexture occlusionTexture;
    DecodedTexture emissiveTexture;

    // Scalar parameters
    float metallic = 0.0f;
    float roughness = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);

    bool alphaMask = false;
    float alphaCutoff = 0.5f;

    std::string name;
};

struct DecodedModel {
    std::unique_ptr<IVertexData> vertices;
    std::vector<uint32_t> indices;
    std::vector<Primitive> primitives;
    std::vector<DecodedMaterial> materials;
    BoundingBox aabb;

    std::string name;
};



struct Material {
    // References to textures IDs from TextureManager
    TextureManager::TextureID albedoTexture = 0;
    TextureManager::TextureID normalTexture = 0;
    TextureManager::TextureID metallicRoughnessTexture = 0;
    TextureManager::TextureID occlusionTexture = 0;
    TextureManager::TextureID emissiveTexture = 0;

    // Scalar parameters
    float metallic = 0.0f;
    float roughness = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f);

    bool alphaMask = false;
    float alphaCutoff = 0.5f;

    std::vector<VkDescriptorSet> descriptorSet; 

    std::string name;
};





class IModelDecoder {
public:
    virtual ~IModelDecoder() = default;

    virtual bool canDecode(const std::filesystem::path& path) const = 0;
    virtual DecodedModel decode(const std::filesystem::path& path) const = 0;
};
