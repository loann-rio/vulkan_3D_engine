#include "Decoder/ImageDecoder.h"
#include "Decoder/STBDecoder.h"
#include "Decoder/HDRDecoder.h"
#include "Decoder/KTX2Decoder.h"
#include "Decoder/KTXDecoder.h"

#include "TextureLoader.h"
#include "TextureUploader.h"

#include <algorithm>
#include <cctype>
#include <future>
#include <stdexcept>
#include <filesystem>
#include "TextureLoader.h"
#include "TextureLoader.h"

//// TextureLoader Implementation ////

/// <summary>
/// Loads a texture from the specified file path and returns a GPU texture by forwarding to the right loader based on file extension
/// </summary>
/// <param name="device">Reference to the Device used to create/upload the texture on the GPU</param>
/// <param name="path">Filesystem path to the texture file</param>
/// <returns>loaded texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::load(Device& device, const std::filesystem::path& path, bool useMipmap) {

    if (path.empty()) {
        throw std::exception("TextureAssetLoader : cannot load : empty texture path");
    }

    const std::string ext = imDecoder::getExtension(path.string());

    if (ext == "hdr") {
        return loadHDR(device, path, useMipmap);
    }
    if (ext == "ktx2") {
        return loadKTX2(device, path, useMipmap);
    }
    if (ext == "ktx") {
        return loadKTX(device, path, useMipmap);
    }

    // PNG/JPG/etc
    return load2D(device, path, useMipmap, /*srgb=*/true);
}

/// <summary>
/// Loads a 2D STB texture 
/// </summary>
/// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
/// <param name="path">Path to the image file to load</param>
/// <param name="srgb">If true, interpret the texture as sRGB; otherwise treat it as linear color space</param>
/// <returns>created Texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::load2D(Device& device, const std::filesystem::path& path, bool useMipmap, bool srgb) {

    STBDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::load2D - Unsupported 2D texture format: " + imDecoder::getExtension(path.string()) + " from file : " + path.string());
    }

    DecodedImage img = decoder.decode(path);
    return loadFromDecoded(device, img, useMipmap, srgb);
}

/// <summary>
/// Loads a 2D HDR texture 
/// </summary>
/// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
/// <param name="path">Path to the image file to load</param>
/// <returns>created Texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadHDR(Device& device, const std::filesystem::path& path, bool useMipmap) {
    HDRDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadHDR - Cannot decode HDR texture: " + path.string());
    }

    DecodedImage img = decoder.decode(path);

    if (!img.isFloat) {
        throw std::runtime_error("TextureLoader::loadHDR - Decoder did not produce floating point image!");
    }

    // HDR = float texture, never sRGB
    return TextureUploader::upload2D(device, img, useMipmap, /*srgb=*/ false);
}

/// <summary>
/// Loads a 2D KTX texture 
/// </summary>
/// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
/// <param name="path">Path to the image file to load</param>
/// <returns>created Texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadKTX(Device& device, const std::filesystem::path& path, bool useMipmap)
{
    KTXDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadKTX - Cannot decode KTX texture: " + path.string());
    }

    DecodedImage img = decoder.decode(path);

    // KTX contains its own format -> no srgb flag needed
    return TextureUploader::uploadCompressed2D(device, img);

}

/// <summary>
/// Loads a 2D KTX2 texture 
/// </summary>
/// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
/// <param name="path">Path to the image file to load</param>
/// <returns>created Texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadKTX2(Device& device, const std::filesystem::path& path, bool useMipmap)
{
    KTX2Decoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadKTX2 - Cannot decode KTX2 texture: " + path.string());
    }

    DecodedImage img = decoder.decode(path);

    // KTX2 contains its own format -> no srgb flag needed
    return TextureUploader::uploadCompressed2D(device, img);
}

/// <summary>
/// Loads a cubemap texture
/// </summary>
/// <param name="device">Device used to create GPU resources and upload the texture.</param>
/// <returns>loaded cubemap texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadCubemap(Device& device, const std::filesystem::path& path) {

    const std::string ext = imDecoder::getExtension(path.string());

    if (ext == "hdr") {
        throw std::runtime_error("Cannot decode hdr as cubemap texture: not implemented yet");
    }
    if (ext == "ktx2") {
        KTX2Decoder decoder;
        DecodedCubemap cubemap = decoder.decodeCubemap(path);
        return loadFromDecoded(device, cubemap, /*srgb=*/false);
    }
    if (ext == "ktx") {
        KTXDecoder decoder;
        DecodedCubemap cubemap = decoder.decodeCubemap(path);
        return loadFromDecoded(device, cubemap, /*srgb=*/false);
    }

    return loadCubemapFromDir(device, path);
}

/// <summary>
/// Loads a cubemap texture from a directory
/// </summary>
/// <param name="device">Device used to create GPU resources and upload the texture</param>
/// <param name="directoryPath">Filesystem path to a directory containing the cubemap image files</param>
/// <returns> loaded cubemap texture </returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadCubemapFromDir(Device& device, const std::filesystem::path& directoryPath)
{
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Path is not a directory: " + directoryPath.string() + " cubemap should be directory or ktx/ktx2");
    }

    STBDecoder decoder;
    DecodedCubemap cubemap = decoder.decodeCubemapFromDirectory(directoryPath.string());
    return loadFromDecoded(device, cubemap, /*srgb=*/false);
}


/// <summary>
/// Creates and uploads a Texture from a decoded image
/// </summary>
std::unique_ptr<TextureObject> TextureAssetLoader::loadFromDecoded(Device& device, const DecodedImage& img, bool useMipmap, bool srgb) {
    if (img.isFloat) {
        // Float -> HDR or EXR
        return TextureUploader::upload2D(device, img, useMipmap, false);
    }
    return TextureUploader::upload2D(device, img, useMipmap, srgb);
}

/// <summary>
/// Creates and uploads a cubemap texture from decoded image data and returns the resulting Texture object
/// </summary>
/// <param name="cube">const reference to the DecodedCubemap containing the six faces image data to upload</param>
/// <returns>uploaded cubemap Texture</returns>
std::unique_ptr<TextureObject> TextureAssetLoader::loadFromDecoded(Device& device, const DecodedCubemap& cube, bool srgb) {
    return TextureUploader::uploadCubemap(device, cube);
}

//// async loading ////
/// TODO
std::future<std::unique_ptr<TextureObject>> TextureAssetLoader::loadAsync(Device& device, const std::filesystem::path& path, bool srgb) {
    return std::async(std::launch::async, [&device, path, srgb]() {
        return load2D(device, path, srgb);
        });
}

