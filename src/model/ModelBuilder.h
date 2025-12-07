#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Model.h"
#include "../base/Device.h"
#include "ModelAsset.h"

class Device;
class ModelManager;



class ModelBuilder {
    enum class SourceType { None, GlTF, Obj };

public:
    explicit ModelBuilder(Device& device);

    //// Input sources ////
    ModelBuilder& fromFile(const std::string& path);
    ModelBuilder& fromObj(const std::string& path);
    ModelBuilder& fromGlTF(const std::string& path);
   
    //// Model options ////
	ModelBuilder& withTexture(const std::string& texturePath);
	ModelBuilder& withMultipleInstances(const std::vector<Model::Instance>& instances);

private:

    //// Hash for caching ////
    uint64_t hash() const;

    //// Build ////
    std::unique_ptr<ModelAsset> build();

    //// Build helpers ////

    Device& device;
    std::string path;

    // Selected decoder type
    SourceType source = SourceType::None;

    friend ModelManager;
};
