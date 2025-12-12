#include "ModelBuilder.h"

#include <algorithm> 
#include <stdexcept>
#include <iostream>

#include "Decoder/ObjModelDecoder.h"
#include "ModelUploader.h"

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
    else if (ext == "gltf")
    {
        if (source != SourceType::GlTF && source != SourceType::None)
            throw std::runtime_error("all model should have the same type");

        source = SourceType::GlTF;
    }

    modelPath.push_back(path);
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
    modelPath.push_back(path);
    return *this;    
}

ModelBuilder& ModelBuilder::fromGlTF(const std::string& path)
{
    if (source != SourceType::GlTF && source != SourceType::None)
        throw std::runtime_error("all model should have the same type");

    auto ext = getExtension(path);
    if (ext != "gltf")
        throw std::runtime_error("not compatible type");

    source = SourceType::GlTF;
    modelPath.push_back(path);
    return *this;
}

ModelBuilder& ModelBuilder::withTexture(TextureManager::TextureID texture)
{
    textures.push_back(texture);
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

    for (auto path: modelPath)
        combine(h, strHash(path));

    for (auto tex : textures)
        combine(h, intHash(tex));

    return h;
}

std::unique_ptr<ModelAsset> ModelBuilder::build()
{
    try {
        switch (source)
        {
            case ModelBuilder::SourceType::GlTF:
                return buildGlTF();

            case ModelBuilder::SourceType::Obj:
                return buildObj();
        
            case ModelBuilder::SourceType::None:
            default:
                throw std::runtime_error("no suported file provided");
        }

    }
    catch (const std::exception& e)
    {
        std::cerr << std::string("TextureBuilder: build failed: ") + e.what() << "\n";
        return nullptr;
    }
}

std::unique_ptr<ModelAsset> ModelBuilder::buildObj()
{
    if (modelPath.empty()) {
        throw std::runtime_error("path cannot be empty for obj");
    }

    std::unique_ptr<ModelAsset> fullModel = std::make_unique<ModelAsset>();

    size_t i = 0;
    for (auto modelFilePath : modelPath)
    {
        if (modelFilePath.empty()) {
            throw std::runtime_error("path cannot be empty for obj");
        }

        ObjModelDecoder decoder;
        if (!decoder.canDecode(modelFilePath)) {
            throw std::runtime_error("obj not suported by decoder");
        }

        DecodedModel decodedModel = decoder.decode(modelFilePath);


        ModelLOD model = ModelUploader::uploadDecodedModel(device, assets, decodedModel);

        if (i < textures.size())
        {
            Material mat;
            mat.albedoTexture = textures[i++];
            model.materials.push_back(mat);
        }

        if (model.materials.empty()) {
            Material mat;
            TextureBuilder builder(device);
            mat.albedoTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
            model.materials.push_back(mat);
        }

        

        fullModel->lods.push_back(std::move(model));
    }

    return fullModel;

}

std::unique_ptr<ModelAsset> ModelBuilder::buildGlTF()
{

    if (modelPath.empty()) {
        throw std::runtime_error("path cannot be empty for obj");
    }

    std::unique_ptr<ModelAsset> fullModel = std::make_unique<ModelAsset>();

    size_t i = 0;
    for (auto modelFilePath : modelPath)
    {
        if (modelFilePath.empty()) {
            throw std::runtime_error("path cannot be empty for obj");
        }

        throw std::runtime_error("build type not implemented yet");

        ObjModelDecoder decoder;
        if (!decoder.canDecode(modelFilePath)) {
            throw std::runtime_error("obj not suported by decoder");
        }

        DecodedModel decodedModel = decoder.decode(modelFilePath);


        ModelLOD model = ModelUploader::uploadDecodedModel(device, assets, decodedModel);

        if (i < textures.size())
        {
            Material mat;
            mat.albedoTexture = textures[i++];
            model.materials.push_back(mat);
        }

        if (model.materials.empty()) {
            Material mat;
            TextureBuilder builder(device);
            mat.albedoTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
            model.materials.push_back(mat);
        }



        fullModel->lods.push_back(std::move(model));
    }

    return fullModel;
    
}



