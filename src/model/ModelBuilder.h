#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ModelAsset.h"


class Device;
class ModelManager;
class AssetManager;

class ModelBuilder {
    enum class SourceType { None, GlTF, Obj, Decoded };

public:
    explicit ModelBuilder(Device& device, AssetManager& assets);

    //// Input sources ////
    ModelBuilder& fromFile(const std::string& path);
    ModelBuilder& fromObj(const std::string& path);
    ModelBuilder& fromGlTF(const std::string& path);
	ModelBuilder& fromDecodedModel(DecodedModel decodedModel);
   
    //// Model options ////
	ModelBuilder& withTexture(TextureManager::TextureID texture);

    ModelBuilder& withShadow(bool enable);

private:

    //// Hash for caching ////
    uint64_t hash() const;

    //// Build ////
    std::unique_ptr<ModelAsset> build();
    
    std::unique_ptr<ModelAsset> buildObj();
    std::unique_ptr<ModelAsset> buildGlTF();
    std::unique_ptr<ModelAsset> buildDecodedModel();

    std::vector<std::string> modelPath{};
    std::vector<TextureManager::TextureID> textures{};

    Device& device;
    AssetManager& assets;

    // Selected decoder type
    SourceType source = SourceType::None;

    DecodedModel decodedModel;

	bool computeShadow = true;

    friend ModelManager;
};
