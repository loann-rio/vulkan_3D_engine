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
    // Automatically picks the correct loader based on file extension
    static std::unique_ptr<TextureObject> load(Device& device, const std::string& path, bool useMipmap);
    static std::unique_ptr<TextureObject> loadCubemap(Device& device, const std::string& Path);

    // Explicit loaders
    static std::unique_ptr<TextureObject> load2D(Device& device, const std::string& path, bool useMipmap, bool srgb = true);
    static std::unique_ptr<TextureObject> loadHDR(Device& device, const std::string& path, bool useMipmap);
    static std::unique_ptr<TextureObject> loadKTX(Device& device, const std::string& path, bool useMipmap);
    static std::unique_ptr<TextureObject> loadKTX2(Device& device, const std::string& path, bool useMipmap);

    // Async version (decode in background, upload on main thread)
    static std::future<std::unique_ptr<TextureObject>> loadAsync(Device& device, const std::string& path, bool srgb = true);

private:
    static std::unique_ptr<TextureObject> loadCubemapFromDir(Device& device, const std::string& directoryPath);

    static std::unique_ptr<TextureObject> loadFromDecoded(Device& device, const DecodedImage& img, bool useMipmap, bool srgb);
    static std::unique_ptr<TextureObject> loadFromDecoded(Device& device, const DecodedCubemap& cube, bool srgb);
};

