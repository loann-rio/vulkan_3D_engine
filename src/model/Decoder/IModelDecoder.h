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


enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };

struct ToBeDecodedTexture {
    std::string textureName;
    std::vector<unsigned char> rawData;
    uint32_t width;
    uint32_t height;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
};

struct DecodedMaterial {
    AlphaMode alphaMode = ALPHAMODE_OPAQUE;

    // References to textures IDs from TextureManager
    std::string albedoTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;

    // potencial index of texture in texture buffer
    int baseColorTextureIndex         { -1 };
    int metallicRoughnessTextureIndex { -1 };
    int normalTextureIndex            { -1 };
    int occlusionTextureIndex         { -1 };
    int emissiveTextureIndex          { -1 };

    // Scalar parameters
    float metallic = 0.0f;
    float roughness = 1.0f;
    float alphaCutoff = 1.0f;

    bool doubleSided = false;

    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    glm::vec4 emissiveFactor = glm::vec4(0.0f);

    int index = 0;
    bool unlit = false;
    float emissiveStrength = 1.0f;
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
    float alphaCutoff = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

    bool doubleSided = false;

    std::vector<VkDescriptorSet> descriptorSet; 

    std::string name;
};

struct DecodedSkin {
    std::string name;
    size_t skeletonRootIndex;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<size_t> jointsIndex;
};

struct DecodedAnimationChannel
{
    enum PathType { TRANSLATION, ROTATION, SCALE };
    PathType path;
    size_t nodeIndex;
    uint32_t samplerIndex;
};

struct DecodedAnimationSampler
{
    enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;
    std::vector<float> outputs;
};

struct DecodedAnimation
{
    std::string name;
    std::vector<DecodedAnimationSampler> samplers;
    std::vector<DecodedAnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
};


struct DecodedModel {
    // Disable copying
    DecodedModel() = default;
    DecodedModel(const DecodedModel&) = delete;
    DecodedModel& operator=(const DecodedModel&) = delete;

    // Allow moving
    DecodedModel(DecodedModel&&) noexcept = default;
    DecodedModel& operator=(DecodedModel&&) noexcept = default;

    // nodes
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<size_t> rootNodes;

    //mesh
    std::unique_ptr<IVertexData> vertices;
    std::vector<uint32_t> indices;

    //materials
    std::vector<DecodedMaterial> materials;
    std::vector<ToBeDecodedTexture> textures;

    // anim + skin
    std::vector<DecodedAnimation> animations;
    std::vector<DecodedSkin> skins;

    BoundingBox aabb;

    std::string name;
};



class IModelDecoder {
public:
    virtual ~IModelDecoder() = default;

    virtual bool canDecode(const std::filesystem::path& path) const = 0;
    virtual DecodedModel decode(const std::filesystem::path& path) const = 0;
};
