#pragma once

//#include "TextureObject.h"

#include <vulkan/vulkan.h>
#include <string>

#include <memory>

class Device;
class TextureObject;

class TextureBuilder {
    enum class SourceType { None, STB, HDR, KTX2, KTX };

public:
    explicit TextureBuilder(Device& device);

    //// Input sources ////
    TextureBuilder& fromFile(const std::string& path);
    TextureBuilder& fromKTX2(const std::string& path);
    TextureBuilder& fromKTX(const std::string& path);
    TextureBuilder& fromHDR(const std::string& path);
    TextureBuilder& fromSTB(const std::string& path);

    //// Texture options ////
    TextureBuilder& withSRGB(bool enable);
    TextureBuilder& withMipmaps(bool enable);
    TextureBuilder& asCubemap(bool enable = true);

    //// Sampler options ////
    TextureBuilder& withMinFilter(VkFilter f);
    TextureBuilder& withMagFilter(VkFilter f);
    TextureBuilder& withWrap(VkSamplerAddressMode mode);

    //// Build ////
    std::unique_ptr<TextureObject> build();

    std::unique_ptr<TextureObject> build2D();
    std::unique_ptr<TextureObject> buildCubemap();


private:
    Device& device;
    std::string path;

    bool forceCubemap = false;
    bool useSRGB = false;
    bool useMipmaps = false;

    VkFilter minFilter = VK_FILTER_LINEAR;
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkSamplerAddressMode wrapMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Selected decoder type
    SourceType source = SourceType::None;
};
