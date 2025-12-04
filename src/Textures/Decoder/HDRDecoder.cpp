#include "HDRDecoder.h"

#include "../../external/stb/stb_image.h"

#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <cctype>

/// <summary>
/// check is the decoder can decode the given file
/// </summary>
bool HDRDecoder::canDecode(const std::string& path) const
{
    return imDecoder::getExtension(path) == "hdr";
}

/// <summary>
/// Loads an image from the given file path using stb_image and returns it as a DecodedImage with 32-bit RGBA pixels
/// </summary>
/// <param name="path">path to the image file to load</param>
/// <returns>DecodedImage</returns>
DecodedImage HDRDecoder::decode(const std::string& path) const
{
    DecodedImage img{};
    img.isFloat = true;

    int width = 0;
    int height = 0;
    int channels = 0;

    float* data = stbi_loadf(
        path.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!data) {
        throw std::runtime_error("HDRDecoder: Failed to load HDR image: " + path);
    }

    img.width = static_cast<uint32_t>(width);
    img.height = static_cast<uint32_t>(height);
    img.channels = 4;

    const size_t floatCount = static_cast<size_t>(width) * height * 4;
    const size_t sizeBytes = floatCount * sizeof(float);

    img.pixels32 = std::vector<float>(floatCount);
    std::memcpy(img.pixels32.data(), data, sizeBytes);

    stbi_image_free(data);
    return img;
}
