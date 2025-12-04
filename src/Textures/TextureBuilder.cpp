#include "TextureBuilder.h"

#include "../base/Device.h"
#include "Decoder/ImageDecoder.h"

#include "TextureLoader.h"
#include "TextureUploader.h"
#include "TextureObject.h"

#include <stdexcept>

TextureBuilder::TextureBuilder(Device& device)
    : device(device)
{}

//// Input sources ////

/// <summary>
/// small setter for file path, save the extension
/// </summary>
/// <param name="p"></param>
/// <returns></returns>
TextureBuilder& TextureBuilder::fromFile(const std::string& p)
{
    path = p;

    // Basic file extension check
    const std::string ext = imDecoder::getExtension(path);

    if (ext == "ktx2")
        source = SourceType::Ktx2;
    else if (ext == "ktx")
        source = SourceType::Ktx1;
    else if (ext == "hdr")
        source = SourceType::Hdr;
    else
        source = SourceType::Stb;

    return *this;
}

/// <summary>
/// small setter for KTX2 file path
/// </summary>
TextureBuilder& TextureBuilder::fromKTX2(const std::string& p)
{
    path = p;
    source = SourceType::Ktx2;
    return *this;
}

/// <summary>
/// small setter for KTX file path
/// </summary>
TextureBuilder& TextureBuilder::fromKTX(const std::string& p)
{
    path = p;
    source = SourceType::Ktx1;
    return *this;
}

/// <summary>
/// small setter for HDR file path
/// </summary>
TextureBuilder& TextureBuilder::fromHDR(const std::string& p)
{
    path = p;
    source = SourceType::Hdr;
    return *this;
}

/// <summary>
/// small setter for STB file path
/// </summary>
TextureBuilder& TextureBuilder::fromSTB(const std::string& p)
{
    path = p;
    source = SourceType::Stb;
    return *this;
}


//// Options ////

TextureBuilder& TextureBuilder::fromVector(const std::vector<std::vector<std::vector<float>>>& textureArray)
{
    source = SourceType::FloatArray;

    arrayH = static_cast<uint32_t>(textureArray.size());
    arrayW = static_cast<uint32_t>(textureArray[0].size());
    arrayD = static_cast<uint32_t>(textureArray[0][0].size());

    if (arrayH == 0 || arrayW == 0 || arrayD == 0)
		throw std::runtime_error("TextureBuilder: fromVector: input array has invalid dimensions");

    arrayPixels.resize(arrayW * arrayH * arrayD);

    size_t index = 0;
    for (size_t y = 0; y < arrayH; ++y) {
        for (size_t x = 0; x < arrayW; ++x) {
            for (size_t c = 0; c < arrayD; ++c) {
                arrayPixels[index++] = textureArray[y][x][c];
            }
        }
    }

    return *this;
}

TextureBuilder& TextureBuilder::fromCharBuffer(std::vector<unsigned char> buffer, const size_t width, const size_t height, const size_t channel, const size_t mipLevel)
{
    charBuffer = buffer;

    arrayH = static_cast<uint32_t>(height);
    arrayW = static_cast<uint32_t>(width);
    arrayD = static_cast<uint32_t>(channel);
	arrayMipLevels = static_cast<uint32_t>(mipLevel);

	if (arrayMipLevels) useMipmaps = true;

	source = SourceType::RawBuffer;
	return *this;
}

/// <summary>
/// small setter to enable/disable sRGB sampling
/// </summary>
TextureBuilder& TextureBuilder::withSRGB(bool enable)
{
    useSRGB = enable;
    return *this;
}

/// <summary>
/// small setter to enable/disable mipmaps
/// </summary>
TextureBuilder& TextureBuilder::withMipmaps(bool enable)
{
    useMipmaps = enable;
    return *this;
}

/// <summary>
/// small setter to force cubemap creation
/// </summary>
TextureBuilder& TextureBuilder::asCubemap(bool enable)
{
    forceCubemap = enable;
    return *this;
}

/// <summary>
/// small setter for minification filter
/// </summary>
TextureBuilder& TextureBuilder::withMinFilter(VkFilter f)
{
    minFilter = f;
    return *this;
}

/// <summary>
/// small setter for magnification filter
/// </summary>
TextureBuilder& TextureBuilder::withMagFilter(VkFilter f)
{
    magFilter = f;
    return *this;
}

/// <summary>
/// small setter for sampler wrap mode
/// </summary>
TextureBuilder& TextureBuilder::withWrap(VkSamplerAddressMode mode)
{
    wrapMode = mode;
    return *this;
}

/// <summary>
/// builds the texture based on the set options
/// </summary>
std::unique_ptr<TextureObject> TextureBuilder::build()
{
    switch (source) {
    case SourceType::RawBuffer: return buildFromCharBuffer();
    case SourceType::FloatArray: return buildFromArray();
    case SourceType::Stb:
    case SourceType::Hdr:
    case SourceType::Ktx1:
    case SourceType::Ktx2:
        return forceCubemap ? buildCubemap() : build2D();
	default: throw std::runtime_error("TextureBuilder: No valid source set");
    }
}

/// <summary>
/// builds a 2D texture
/// </summary>
std::unique_ptr<TextureObject> TextureBuilder::build2D()
{
    if (path.empty())
        throw std::runtime_error("TextureBuilder: No input path set");

	auto texture = TextureLoader::load(device, path, useMipmaps);

    // Update sampler parameters
    texture->updateSampler(minFilter, magFilter, wrapMode);

    return texture;
}

/// <summary>
/// builds a cubemap texture
/// </summary>
std::unique_ptr<TextureObject> TextureBuilder::buildCubemap()
{
    if (path.empty())
        throw std::runtime_error("TextureBuilder: No input path set");
    
    auto texture = TextureLoader::loadCubemap(device, path);

    // Update sampler parameters
    texture->updateSampler(minFilter, magFilter, wrapMode);

	return texture;
}

std::unique_ptr<TextureObject> TextureBuilder::buildFromArray()
{
    if (arrayPixels.empty())
        throw std::runtime_error("TextureBuilder: Array source is empty");

    DecodedImage img{};
    img.width = arrayW;
    img.height = arrayH;
    img.channels = arrayD;
    img.isFloat = true;
    img.isCompressed = false;
    img.pixels32 = arrayPixels;
    img.mipLevels = 1;

    img.format = (arrayD == 1) ? VK_FORMAT_R32_SFLOAT :
        (arrayD == 2) ? VK_FORMAT_R32G32_SFLOAT :
        (arrayD == 3) ? VK_FORMAT_R32G32B32_SFLOAT :
        VK_FORMAT_R32G32B32A32_SFLOAT;

    return TextureUploader::upload2D(device, img, false, useSRGB);
}

std::unique_ptr<TextureObject> TextureBuilder::buildFromCharBuffer()
{
    if (charBuffer.empty())
        throw std::runtime_error("TextureBuilder: char buffer source is empty");

    DecodedImage img{};
    img.width = arrayW;
    img.height = arrayH;
    img.channels = arrayD;
    img.isFloat = false;
    img.isCompressed = false;
	img.pixels8 = charBuffer;
    img.mipLevels = arrayMipLevels;

    img.format = (arrayD == 1) ? VK_FORMAT_R8_UNORM :
        (arrayD == 2) ? VK_FORMAT_R8G8_UNORM :
        (arrayD == 3) ? VK_FORMAT_R8G8B8_UNORM :
        VK_FORMAT_R8G8B8A8_UNORM;
    
    return TextureUploader::upload2D(device, img, useMipmaps, useSRGB);
}

std::unique_ptr<TextureObject> TextureBuilder::fromTextureInfo(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount)
{
	std::unique_ptr<TextureObject> texture = std::make_unique<TextureObject>(device);
    texture->textureExtent = { imageInfo.extent.width, imageInfo.extent.height };

    // create image
    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->textureImage, texture->textureImageMemory);
    // create image view
    viewInfo.image = texture->textureImage;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &texture->textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    // create sampler
    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &texture->textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    if (initImageLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        device.transitionImageLayout(texture->textureImage, imageInfo.format,
            VK_IMAGE_LAYOUT_UNDEFINED, initImageLayout, layerCount);

	texture->isLoaded = true;
    return texture;
}
