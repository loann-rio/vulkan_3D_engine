#include "TextureBuilder.h"

#include "../base/Device.h"
#include "Decoder/ImageDecoder.h"

#include "TextureLoader.h"
#include "TextureUploader.h"
#include "TextureObject.h"

#include <stdexcept>
#include <iostream>

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
    try {
        switch (source) {
        case SourceType::RawBuffer: return buildFromCharBuffer();
        case SourceType::FloatArray: return buildFromArray();
        case SourceType::Custom: return std::move(existingTexture);
        case SourceType::Stb:
        case SourceType::Hdr:
        case SourceType::Ktx1:
        case SourceType::Ktx2:
            return forceCubemap ? buildCubemap() : build2D();
        default: throw std::runtime_error("TextureBuilder: No valid source set");
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << std::string("TextureBuilder: build failed: ") + e.what() << "\n";
        return nullptr;
	}   
}

/// <summary>
/// builds a 2D texture
/// </summary>
std::unique_ptr<TextureObject> TextureBuilder::build2D()
{
    if (path.empty())
        throw std::runtime_error("TextureBuilder: No input path set");

	auto texture = TextureAssetLoader::load(device, path, useMipmaps);

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
    
    auto texture = TextureAssetLoader::loadCubemap(device, path);

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

TextureBuilder& TextureBuilder::fromTextureInfo(VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount)
{
    source = SourceType::Custom;

    existingTexture = std::make_unique<TextureObject>(device);
    existingTexture->textureExtent = { imageInfo.extent.width, imageInfo.extent.height };

    // create image
    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, existingTexture->textureImage, existingTexture->textureImageMemory);
    // create image view
    viewInfo.image = existingTexture->textureImage;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &existingTexture->textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    // create sampler
    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &existingTexture->textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    if (initImageLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        device.transitionImageLayout(existingTexture->textureImage, imageInfo.format,
            VK_IMAGE_LAYOUT_UNDEFINED, initImageLayout, layerCount);

    existingTexture->isLoaded = true;
    return *this;
}

uint64_t TextureBuilder::hash() const
{
    if (source == SourceType::Custom || source == SourceType::RawBuffer || source == SourceType::FloatArray)
    {
		// Generate a unique id for custom textures
        static std::atomic<uint64_t> customCounter = 1;
        return 0xFFFFFFFF00000000ull | customCounter++;
    }

    auto combine = [](uint64_t& seed, uint64_t v) {
        seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

    // simple hash combining path and options
    std::hash<std::string>      strHash;
    std::hash<bool>             boolHash;
    std::hash<uint32_t>         u32Hash;
    std::hash<unsigned char>    u8Hash;
    std::hash<float>            fHash;
    std::hash<int>              intHash;

    uint64_t h = 0;

    combine(h, intHash(static_cast<int>(source)));

    combine(h, boolHash(useSRGB));
    combine(h, boolHash(useMipmaps));
    combine(h, intHash(minFilter));
    combine(h, intHash(magFilter));
    combine(h, intHash(wrapMode));
    combine(h, boolHash(forceCubemap));

    if (source == SourceType::Stb ||
        source == SourceType::Hdr ||
        source == SourceType::Ktx1 ||
        source == SourceType::Ktx2)
    {
        combine(h, strHash(path));
        return h;
    }

    return h;

}