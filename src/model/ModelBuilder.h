#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "ModelAsset.h"
#include "Decoder/IModelDecoder.h"
#include "../assetManager/TextureManager.h"


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
    std::unique_ptr<ModelAsset> build(const std::string& targetScene);
    
    DecodedModel decodeObj();
    DecodedModel decodeGlTF();

    bool decodeModel();
    void uploadModel(ModelAsset* model, const std::string& targetScene);
    
    std::string modelPath = "";
    TextureManager::TextureID texture = -1;

    Device& device;
    AssetManager& assets;

    // Selected decoder type
    SourceType source = SourceType::None;

    DecodedModel decodedModel;

	bool computeShadow = true;

    friend ModelManager;
};
