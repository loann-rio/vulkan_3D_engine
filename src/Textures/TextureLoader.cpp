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

//// TextureLoader Implementation ////

/// <summary>
/// Loads a texture from the specified file path and returns a GPU texture wrapped in a std::unique_ptr. The loader chooses the loading path based on the file extension: .hdr files are handled by loadHDR, .ktx2 files are decoded with KTX2Decoder and uploaded with srgb=false, and other formats (PNG/JPG/etc.) are loaded via load2D with srgb=true.
/// </summary>
/// <param name="device">Reference to the Device used to create/upload the texture on the GPU.</param>
/// <param name="path">Filesystem path to the texture file. The function inspects the file extension (e.g., .hdr, .ktx2, .png, .jpg) to determine which loader/decoder to use.</param>
/// <returns>A std::unique_ptr<Texture> owning the loaded texture. Ownership is transferred to the caller. May be nullptr if the texture could not be loaded.</returns>
std::unique_ptr<TextureObject> TextureLoader::load(Device& device, const std::string& path, bool useMipmap) {
    const std::string ext = imDecoder::getExtension(path);

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
/// Loads a 2D texture from the given file path, decoding the image data and creating a Texture. Uses an HDR loader for .hdr files and STBDecoder for other supported formats; throws if the format is unsupported
/// </summary>
/// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
/// <param name="path">Path to the image file to load</param>
/// <param name="srgb">If true, interpret the texture as sRGB; otherwise treat it as linear color space</param>
/// <returns>A std::unique_ptr owning the created Texture</returns>
std::unique_ptr<TextureObject> TextureLoader::load2D(Device& device, const std::string& path, bool useMipmap, bool srgb) {

    STBDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::load2D - Unsupported 2D texture format: " + imDecoder::getExtension(path));
    }

    DecodedImage img = decoder.decode(path);
    return loadFromDecoded(device, img, useMipmap, srgb);
}

/// <summary>
/// Loads an HDR image from the given file path, verifies it can be decoded as a floating-point image, uploads it to the device as an HDR (floating-point, not sRGB) texture, and returns the created texture. 
/// Throws runtime_error if decoding is not possible or the decoder does not produce a floating-point image
/// </summary>
/// <param name="device">Reference to the Device used to create/upload the texture resources</param>
/// <param name="path">Filesystem path to the HDR image to load</param>
/// <returns>A std::unique_ptr<Texture> owning the uploaded HDR (floating-point) texture</returns>
std::unique_ptr<TextureObject> TextureLoader::loadHDR(Device& device, const std::string& path, bool useMipmap) {
    HDRDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadHDR - Cannot decode HDR texture: " + path);
    }

    DecodedImage img = decoder.decode(path);

    if (!img.isFloat) {
        throw std::runtime_error("TextureLoader::loadHDR - Decoder did not produce floating point image!");
    }

    // HDR = float texture, never sRGB
    return TextureUploader::upload2D(device, img, useMipmap, /*srgb=*/ false);
}

std::unique_ptr<TextureObject> TextureLoader::loadKTX(Device& device, const std::string& path, bool useMipmap)
{
    KTXDecoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadKTX - Cannot decode KTX texture: " + path);
    }

    DecodedImage img = decoder.decode(path);

    // KTX contains its own format -> no srgb flag needed
    return TextureUploader::upload2D(device, img, true, /*srgb=*/ false);

}

std::unique_ptr<TextureObject> TextureLoader::loadKTX2(Device& device, const std::string& path, bool useMipmap)
{
    KTX2Decoder decoder;
    if (!decoder.canDecode(path)) {
        throw std::runtime_error("TextureLoader::loadKTX2 - Cannot decode KTX2 texture: " + path);
    }

    DecodedImage img = decoder.decode(path);

    // KTX2 contains its own format -> no srgb flag needed
    return TextureUploader::upload2D(device, img, true, /*srgb=*/ false);
}

/// <summary>
/// Loads a cubemap texture
/// </summary>
/// <param name="device">Device used to create GPU resources and upload the texture.</param>
/// <returns>A std::unique_ptr<Texture> owning the loaded cubemap texture</returns>
std::unique_ptr<TextureObject> TextureLoader::loadCubemap(Device& device, const std::string& path) {

    const std::string ext = imDecoder::getExtension(path);

    if (ext == ".hdr") {
        throw std::runtime_error("Cannot decode hdr as cubemap texture: not implemented yet");
    }
    if (ext == ".ktx2") {
        KTX2Decoder decoder;
        DecodedCubemap cubemap = decoder.decodeCubemap(path);
		return loadFromDecoded(device, cubemap, /*srgb=*/false);
    }
    if (ext == ".ktx") {
        KTXDecoder decoder;
        DecodedCubemap cubemap = decoder.decodeCubemap(path);
        return loadFromDecoded(device, cubemap, /*srgb=*/false);
    }

    return loadCubemapFromDir(device, path);
}

/// <summary>
/// Loads a cubemap texture from a directory: verifies the path is a directory, decodes the cubemap data, and creates a Texture using sRGB color space.
/// </summary>
/// <param name="device">Device used to create GPU resources and upload the texture.</param>
/// <param name="directoryPath">Filesystem path to a directory containing the cubemap image files.</param>
/// <returns>A std::unique_ptr<Texture> owning the loaded cubemap texture (created with sRGB). May throw std::runtime_error if the path does not exist or is not a directory; other errors from decoding or texture creation may propagate.</returns>
std::unique_ptr<TextureObject> TextureLoader::loadCubemapFromDir(Device& device, const std::string& directoryPath)
{
    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
        throw std::runtime_error("Path is not a directory: " + directoryPath + " cubemap should be directory or ktx/ktx2");
    }

    STBDecoder decoder;
    DecodedCubemap cubemap = decoder.decodeCubemapFromDirectory(directoryPath);
    return loadFromDecoded(device, cubemap, /*srgb=*/false);
}

//// Internal glue functions ////

/// <summary>
/// Creates and uploads a Texture from a decoded image, using an HDR/EXR upload for floating-point images and a standard 2D upload otherwise.
/// </summary>
/// <param name="device">Reference to the Device used for texture creation and uploading.</param>
/// <param name="img">DecodedImage containing the pixel data and metadata. If img.isFloat is true, an HDR/EXR upload path is used.</param>
/// <param name="srgb">If true and the image is not floating-point, treat the image as sRGB during upload. Ignored for floating-point images.</param>
/// <returns>A std::unique_ptr<Texture> that owns the uploaded texture. May be nullptr on failure.</returns>
std::unique_ptr<TextureObject> TextureLoader::loadFromDecoded(Device& device, const DecodedImage& img, bool useMipmap, bool srgb) {
    if (img.isFloat) {
        // Float -> HDR or EXR
        return TextureUploader::upload2D(device, img, useMipmap, false);
    }
    return TextureUploader::upload2D(device, img, useMipmap, srgb);
}

/// <summary>
/// Creates and uploads a cubemap texture from decoded image data and returns the resulting Texture object
/// </summary>
/// <param name="device">The Device (rendering context) used to create and upload GPU resources for the texture</param>
/// <param name="cube">A const reference to the DecodedCubemap containing the six faces' image data to upload</param>
/// <returns>A std::unique_ptr<Texture> that owns the uploaded cubemap Texture. The returned pointer may be empty if the upload fails</returns>
std::unique_ptr<TextureObject> TextureLoader::loadFromDecoded(Device& device, const DecodedCubemap& cube, bool srgb) {
    return TextureUploader::uploadCubemap(device, cube);
}

//// async loading ////

/// <summary>
/// Starts an asynchronous task to load a 2D texture from a file and returns a future that will hold the loaded texture
/// </summary>
/// <param name="device">Reference to the Device used to create or upload the texture</param>
/// <param name="path">Path to the texture file to load</param>
/// <param name="srgb">If true, interpret the texture data as sRGB; otherwise treat it as linear</param>
/// <returns>A std::future that will hold a std::unique_ptr<Texture> pointing to the loaded texture</returns>
std::future<std::unique_ptr<TextureObject>> TextureLoader::loadAsync(Device& device, const std::string& path, bool srgb) {
    return std::async(std::launch::async, [&device, path, srgb]() {
        return load2D(device, path, srgb);
        });
}