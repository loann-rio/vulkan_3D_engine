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

TextureBuilder& TextureBuilder::fromKTX2(const std::string& p)
{
    path = p;
    source = SourceType::KTX2;
    return *this;
}

TextureBuilder& TextureBuilder::fromKTX(const std::string& p)
{
    path = p;
    source = SourceType::KTX;
    return *this;
}

TextureBuilder& TextureBuilder::fromHDR(const std::string& p)
{
    path = p;
    source = SourceType::HDR;
    return *this;
}

TextureBuilder& TextureBuilder::fromSTB(const std::string& p)
{
    path = p;
    source = SourceType::STB;
    return *this;
}

//// Options ////

TextureBuilder& TextureBuilder::withSRGB(bool enable)
{
    useSRGB = enable;
    return *this;
}

TextureBuilder& TextureBuilder::withMipmaps(bool enable)
{
    useMipmaps = enable;
    return *this;
}

TextureBuilder& TextureBuilder::asCubemap(bool enable)
{
    forceCubemap = enable;
    return *this;
}

TextureBuilder& TextureBuilder::withMinFilter(VkFilter f)
{
    minFilter = f;
    return *this;
}

TextureBuilder& TextureBuilder::withMagFilter(VkFilter f)
{
    magFilter = f;
    return *this;
}

TextureBuilder& TextureBuilder::withWrap(VkSamplerAddressMode mode)
{
    wrapMode = mode;
    return *this;
}

//// Build uncompressed textures ////

std::unique_ptr<TextureObject> TextureBuilder::build()
{
    if (forceCubemap)
        return buildCubemap();

    return build2D();
}

std::unique_ptr<TextureObject> TextureBuilder::build2D()
{
    if (path.empty())
        throw std::runtime_error("TextureBuilder: No input path set");

	auto texture = TextureLoader::load(device, path, useMipmaps);

    // Update sampler parameters
    texture->updateSampler(minFilter, magFilter, wrapMode);

    return texture;
}

std::unique_ptr<TextureObject> TextureBuilder::buildCubemap()
{
    if (path.empty())
        throw std::runtime_error("TextureBuilder: No input path set");
    
    auto texture = TextureLoader::loadCubemap(device, path);

    // Update sampler parameters
    texture->updateSampler(minFilter, magFilter, wrapMode);

	return texture;
}
