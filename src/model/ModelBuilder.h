#pragma once

//#include "TextureObject.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

#include "../model/Model.h"

class Device;


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


    //// Build helpers ////

    Device& device;
    std::string path;

    // from array
    uint32_t arrayW = 0;
    uint32_t arrayH = 0;
    uint32_t arrayD = 0;
    uint32_t arrayMipLevels = 1;
    std::vector<float> arrayPixels;

    // from char buffer
    std::vector<unsigned char> charBuffer;

    // from existing texture
    std::unique_ptr<TextureObject> existingTexture = nullptr;


    bool forceCubemap = false;
    bool useSRGB = false;
    bool useMipmaps = false;

    VkFilter minFilter = VK_FILTER_LINEAR;
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkSamplerAddressMode wrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Selected decoder type
    SourceType source = SourceType::None;

    friend TextureManager;
};

template<size_t W, size_t H, size_t D>
inline TextureBuilder& TextureBuilder::fromArray(const TextureArray<W, H, D>& textureArray)
{
    source = SourceType::Array;

    arrayW = static_cast<uint32_t>(W);
    arrayH = static_cast<uint32_t>(H);
    arrayD = static_cast<uint32_t>(D);
    arrayPixels.resize(W * H * D);

    size_t index = 0;
    for (size_t y = 0; y < H; ++y) {
        for (size_t x = 0; x < W; ++x) {
            for (size_t c = 0; c < D; ++c) {
                arrayPixels[index++] = textureArray[y][x][c];
            }
        }
    }

    return *this;
}

