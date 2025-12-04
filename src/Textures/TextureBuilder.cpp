#include "TextureBuilder.h"

#include "../base/Device.h"
#include "Decoder/ImageDecoder.h"

#include "TextureLoader.h"

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
        source = SourceType::KTX2;
    else if (ext == "ktx")
        source = SourceType::KTX;
    else if (ext == "hdr")
        source = SourceType::HDR;
    else
        source = SourceType::STB;

    return *this;
}

/// <summary>
/// small setter for KTX2 file path
/// </summary>
TextureBuilder& TextureBuilder::fromKTX2(const std::string& p)
{
    path = p;
    source = SourceType::KTX2;
    return *this;
}

/// <summary>
/// small setter for KTX file path
/// </summary>
TextureBuilder& TextureBuilder::fromKTX(const std::string& p)
{
    path = p;
    source = SourceType::KTX;
    return *this;
}

/// <summary>
/// small setter for HDR file path
/// </summary>
TextureBuilder& TextureBuilder::fromHDR(const std::string& p)
{
    path = p;
    source = SourceType::HDR;
    return *this;
}

/// <summary>
/// small setter for STB file path
/// </summary>
TextureBuilder& TextureBuilder::fromSTB(const std::string& p)
{
    path = p;
    source = SourceType::STB;
    return *this;
}

//// Options ////

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
    if (forceCubemap)
        return buildCubemap();

    return build2D();
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
