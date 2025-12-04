#pragma once
#include <memory>
#include <string>
#include <future>

class Device;
class TextureObject;
struct DecodedImage;
struct DecodedCubemap;

class TextureLoader {
public:

    /// <summary>
    /// Loads a texture from the specified file path and returns a GPU texture by forwarding to the right loader based on file extension
    /// </summary>
    /// <param name="device">Reference to the Device used to create/upload the texture on the GPU</param>
    /// <param name="path">Filesystem path to the texture file</param>
    /// <returns>loaded texture</returns>
    static std::unique_ptr<TextureObject> load(
        Device& device, 
        const std::string& path, 
        bool useMipmap
    );

    /// <summary>
    /// Loads a cubemap texture
    /// </summary>
    /// <param name="device">Device used to create GPU resources and upload the texture.</param>
    /// <returns>loaded cubemap texture</returns>
    static std::unique_ptr<TextureObject> loadCubemap(
        Device& device, 
        const std::string& Path
    );

    //// Explicit loaders ////

    /// <summary>
    /// Loads a 2D STB texture 
    /// </summary>
    /// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
    /// <param name="path">Path to the image file to load</param>
    /// <param name="srgb">If true, interpret the texture as sRGB; otherwise treat it as linear color space</param>
    /// <returns>created Texture</returns>
    static std::unique_ptr<TextureObject> load2D(
        Device& device, 
        const std::string& path, 
        bool useMipmap, 
        bool srgb = true
    );

    /// <summary>
    /// Loads a 2D HDR texture 
    /// </summary>
    /// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
    /// <param name="path">Path to the image file to load</param>
    /// <returns>created Texture</returns>
    static std::unique_ptr<TextureObject> loadHDR(
        Device& device, 
        const std::string& path, 
        bool useMipmap
    );

    /// <summary>
    /// Loads a 2D KTX texture 
    /// </summary>
    /// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
    /// <param name="path">Path to the image file to load</param>
    /// <returns>created Texture</returns>
    static std::unique_ptr<TextureObject> loadKTX(
        Device& device, 
        const std::string& path, 
        bool useMipmap
    );

    /// <summary>
    /// Loads a 2D KTX2 texture 
    /// </summary>
    /// <param name="device">Reference to the Device used to create and upload GPU resources for the Texture</param>
    /// <param name="path">Path to the image file to load</param>
    /// <returns>created Texture</returns>
    static std::unique_ptr<TextureObject> loadKTX2(
        Device& device, 
        const std::string& path, 
        bool useMipmap
    );

    // Async version TODO
    static std::future<std::unique_ptr<TextureObject>> loadAsync(
        Device& device, 
        const std::string& path, 
        bool srgb = true
    );

private:
    /// <summary>
    /// Loads a cubemap texture from a directory
    /// </summary>
    /// <param name="device">Device used to create GPU resources and upload the texture</param>
    /// <param name="directoryPath">Filesystem path to a directory containing the cubemap image files</param>
    /// <returns> loaded cubemap texture </returns>
    static std::unique_ptr<TextureObject> loadCubemapFromDir(
        Device& device, 
        const std::string& directoryPath
    );

    /// <summary>
    /// Creates and uploads a Texture from a decoded image
    /// </summary>
    static std::unique_ptr<TextureObject> loadFromDecoded(
        Device& device, 
        const DecodedImage& img, 
        bool useMipmap, 
        bool srgb
    );

    /// <summary>
    /// Creates and uploads a cubemap texture from decoded image data and returns the resulting Texture object
    /// </summary>
    /// <param name="cube">const reference to the DecodedCubemap containing the six faces image data to upload</param>
    /// <returns>uploaded cubemap Texture</returns>
    static std::unique_ptr<TextureObject> loadFromDecoded(
        Device& device, 
        const DecodedCubemap& cube, 
        bool srgb
    );
};

