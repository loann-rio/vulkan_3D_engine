#include "ModelBuilder.h"

#include <algorithm> 
#include <stdexcept>
#include <iostream>
#include <random>
#include <cctype>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "Decoder/ObjModelDecoder.h"
#include "Decoder/GlTFModelDecoder.h"
#include "Decoder/IModelDecoder.h"

#include "ModelUploader.h"
#include "ModelAsset.h"

#include "../assetManager/AssetManager.h"
#include "../assetManager/TextureManager.h"
#include "../base/Device.h"
#include "../Textures/TextureBuilder.h"

namespace {

    /// Return lowercase extension without dot
    std::string getExtension(const std::string& path) {
        auto pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return {};

        std::string ext = path.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return ext;
    }
}

ModelBuilder::ModelBuilder(Device& device, AssetManager& assets) : device(device), assets(assets) {}

ModelBuilder& ModelBuilder::fromFile(const std::string& path)
{

    auto ext = getExtension(path);
    if (ext == "obj")
    {
        if (source != SourceType::Obj && source != SourceType::None) 
            throw std::runtime_error("all model should have the same type");

        source = SourceType::Obj;
    }
    else if (ext == "gltf" || ext == "glb")
    {
        if (source != SourceType::GlTF && source != SourceType::None)
            throw std::runtime_error("all model should have the same type");

        source = SourceType::GlTF;
    }

    modelPath = path;
    return *this;
}

ModelBuilder& ModelBuilder::fromObj(const std::string& path)
{
    if (source != SourceType::Obj && source != SourceType::None)
        throw std::runtime_error("all model should have the same type");

    auto ext = getExtension(path);
    if (ext != "obj")
        throw std::runtime_error("not compatible type");

    source = SourceType::Obj;
    modelPath = path;
    return *this;    
}

ModelBuilder& ModelBuilder::fromGlTF(const std::string& path)
{
    if (source != SourceType::GlTF && source != SourceType::None)
        throw std::runtime_error("all model should have the same type");

    auto ext = getExtension(path);
    if (ext != "gltf" && ext != "glb")
        throw std::runtime_error("not compatible type");

    source = SourceType::GlTF;
    modelPath = path;
    return *this;
}

ModelBuilder& ModelBuilder::fromDecodedModel(DecodedModel decodedModelParam)
{
    if (source != SourceType::Decoded && source != SourceType::None)
        throw std::runtime_error("all model should have the same type");

    source = SourceType::Decoded;
    decodedModel = std::move(decodedModelParam);
    return *this;
}

ModelBuilder& ModelBuilder::withTexture(TextureManager::TextureID _texture)
{
    texture = _texture;
    return *this;
}

ModelBuilder& ModelBuilder::withShadow(bool enable)
{
	computeShadow = enable;
	return *this;
}

uint64_t ModelBuilder::hash() const
{
    auto combine = [](uint64_t& seed, uint64_t v) {
        seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

    std::hash<std::string> strHash;
    std::hash<uint64_t> intHash;

    uint64_t h = 0;

    if (source == SourceType::Decoded) {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        uint64_t rnd = gen();
        combine(h, rnd);
        return h;
    }

    combine(h, strHash(modelPath));
    combine(h, intHash(texture));

    return h;
}

std::unique_ptr<ModelAsset> ModelBuilder::build(const std::string& targetScene)
{
    std::unique_ptr<DecodedModel> decodedModel = decodeModel();
    decodedModel->texture = texture;
    

    Mesh model = ModelUploader::uploadDecodedModel(device, assets, targetScene, *decodedModel);

    if (texture != -1) {
        Material mat;
        mat.albedoTexture = texture;
        model.materials.push_back(mat);
    }

    if (model.materials.empty()) {
        Material mat;
        TextureBuilder builder(device);
        mat.albedoTexture = assets.textures(targetScene).create(builder.fromFile("assets/textures/whiteTexture.jpg"));
        model.materials.push_back(mat);
    }

    ModelUploader::uploadShaderMaterials(device, model);

    std::unique_ptr<ModelAsset> fullModel = std::make_unique<ModelAsset>();
    fullModel->mesh = std::move(model);
    fullModel->hasShadow = computeShadow;
    fullModel->skeleton = std::move(decodedModel->skeleton);

    return fullModel;
}

DecodedModel ModelBuilder::decodeObj()
{
    if (modelPath == "") {
        throw std::runtime_error("path cannot be empty for obj");
    }

    ObjModelDecoder decoder;
    if (!decoder.canDecode(modelPath)) {
        throw std::runtime_error("obj not suported by decoder");
    }

    return decoder.decode(modelPath);
}

DecodedModel ModelBuilder::decodeGlTF()
{

    if (modelPath == "") {
        throw std::runtime_error("path cannot be empty for obj");
    }

    GlTFModelDecoder decoder;
    if (!decoder.canDecode(modelPath)) {
        throw std::runtime_error("obj not suported by decoder");
    }

    return decoder.decode(modelPath);
}

void ModelBuilder::decodeModel()
{
    try {
        switch (source)
        {
            case ModelBuilder::SourceType::GlTF:
                decodedModel = std::move(decodeGlTF());
                decodedModel.texture = texture;
                break;

            case ModelBuilder::SourceType::Obj:
                decodedModel = std::move(decodeObj());
                decodedModel.texture = texture;

                break;

			case ModelBuilder::SourceType::Decoded:
                break;

            case ModelBuilder::SourceType::None:
            default:
                throw std::runtime_error("no suported file provided");
        }

    }
    catch (const std::exception& e)
    {
        std::cerr << std::string("ModelBuilder: build failed: ") + e.what() << "\n";
        return;
    }
}
