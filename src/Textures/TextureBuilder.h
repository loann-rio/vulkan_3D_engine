#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

class Device;
class TextureObject;
class TextureManager;

class TextureBuilder {
    enum class SourceType { None, Stb, Hdr, Ktx2, Ktx1, FloatArray, RawBuffer, Custom };

    template <size_t W, size_t H, size_t D>
    using TextureArray = std::array<std::array<std::array<float, D>, W>, H>;

public:
    explicit TextureBuilder(Device& device);

    //// Input sources ////
    TextureBuilder& fromFile(const std::string& path);
    TextureBuilder& fromKTX2(const std::string& path);
    TextureBuilder& fromKTX(const std::string& path);
    TextureBuilder& fromHDR(const std::string& path);
    TextureBuilder& fromSTB(const std::string& path);

    template <size_t W, size_t H, size_t D>
    TextureBuilder& fromArray(const TextureArray<W, H, D>& textureArray);

    TextureBuilder& fromVector(const std::vector<std::vector<std::vector<float>>>& textureArray);
    TextureBuilder& fromCharBuffer(std::vector<unsigned char> buffer, const size_t width, const size_t height, const size_t channel, const size_t mipLevel);

    //// Texture options ////
    TextureBuilder& withSRGB(bool enable);
    TextureBuilder& withMipmaps(bool enable);
    TextureBuilder& asCubemap(bool enable = true);

    //// Sampler options ////
    TextureBuilder& withMinFilter(VkFilter f);
    TextureBuilder& withMagFilter(VkFilter f);
    TextureBuilder& withWrap(VkSamplerAddressMode mode);

    //// from existing texture ////
    TextureBuilder& fromTextureInfo(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount = 1);
    TextureBuilder& fromTextureInfo(VkImage image, VkExtent3D extent, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount = 1, VkFormat format = VK_FORMAT_UNDEFINED);

private:

    //// Hash for caching ////
    uint64_t hash() const;

    //// Build ////
    std::unique_ptr<TextureObject> build();

    //// Build helpers ////
    std::unique_ptr<TextureObject> build2D();
    std::unique_ptr<TextureObject> buildCubemap();
    std::unique_ptr<TextureObject> buildFromArray();
    std::unique_ptr<TextureObject> buildFromCharBuffer();


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
