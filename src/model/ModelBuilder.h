#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ModelAsset.h"
#include "Vertex/IVertexData.h"


class Device;
class ModelManager;
class AssetManager;

class ModelBuilder {
    enum class SourceType { None, GlTF, Obj, Vertex };

public:
    explicit ModelBuilder(Device& device, AssetManager& assets);

    //// Input sources ////
    ModelBuilder& fromFile(const std::string& path);
    ModelBuilder& fromObj(const std::string& path);
    ModelBuilder& fromGlTF(const std::string& path);

    ModelBuilder& fromVertexList(std::unique_ptr<IVertexData> vertices, std::vector<uint32_t> indices);
   
    //// Model options ////
	ModelBuilder& withTexture(TextureManager::TextureID texture);

private:

    //// Hash for caching ////
    uint64_t hash() const;

    //// Build ////
    std::unique_ptr<ModelAsset> build();
    
    std::unique_ptr<ModelAsset> buildObj();
    std::unique_ptr<ModelAsset> buildGlTF();
    std::unique_ptr<ModelAsset> buildFromVertice();

    std::vector<std::string> modelPath{};
    std::vector<TextureManager::TextureID> textures{};

    std::unique_ptr<IVertexData> vertices;
    std::vector<uint32_t> indices;

    Device& device;
    AssetManager& assets;

    // Selected decoder type
    SourceType source = SourceType::None;

    friend ModelManager;
};
