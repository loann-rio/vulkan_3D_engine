#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ModelAsset.h"


class Device;
class ModelManager;
class AssetManager;

class ModelBuilder {
    enum class SourceType { None, GlTF, Obj };

public:
    explicit ModelBuilder(Device& device, AssetManager& assets);

    //// Input sources ////
    ModelBuilder& fromFile(const std::string& path);
    ModelBuilder& fromObj(const std::string& path);
    ModelBuilder& fromGlTF(const std::string& path);
   
    //// Model options ////
	ModelBuilder& withTexture(TextureManager::TextureID texture);

private:

    //// Hash for caching ////
    uint64_t hash() const;

    //// Build ////
    std::unique_ptr<ModelAsset> build();
    
    std::unique_ptr<ModelAsset> buildObj();
    std::unique_ptr<ModelAsset> buildGlTF();

    std::vector<std::string> modelPath{};
    std::vector<TextureManager::TextureID> textures{};

    Device& device;
    AssetManager& assets;

    // Selected decoder type
    SourceType source = SourceType::None;

    friend ModelManager;
};
