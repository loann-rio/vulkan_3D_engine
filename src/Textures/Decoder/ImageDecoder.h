#pragma once

#include <algorithm> 
#include <memory>
#include <string>   
#include <vector>
#include <array>

#include <vulkan/vulkan.h>

namespace imDecoder {

    /// <summary>
    /// Return lowercase extension without dot
    /// </summary>
    /// <param name="path">path to texture</param>
    /// <returns>lowercase extension without dot</returns>
    inline std::string getExtension(const std::string& path) {
        auto pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return {};

        std::string ext = path.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return ext;
    }
}

struct DecodedImage {
    int width = 0;
    int height = 0;
    int channels = 4;
    int arrayLayers = 1;

    VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags imageFlag = 0;

    bool isFloat = false;
    bool isCompressed = false;
	bool isCubemap = false;

    std::vector<unsigned char> pixels8;
    std::vector<float> pixels32;
    std::vector<uint8_t> compressedData;

	size_t dataSize = 0; 

    std::vector<uint32_t> mipSizes;
    std::vector<uint32_t> mipOffsets;
    uint32_t mipLevels = 1;

};

struct DecodedCubemap {
    // 6 faces in order: +X, -X, +Y, -Y, +Z, -Z
    std::array<DecodedImage, 6> faces;
};

class ImageDecoder {
public:
    virtual ~ImageDecoder() = default;

    virtual bool canDecode(const std::string& path) const = 0;
    virtual DecodedImage decode(const std::string& path) const = 0;
};
