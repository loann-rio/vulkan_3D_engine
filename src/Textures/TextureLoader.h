#pragma once
#include <memory>
#include <string>
#include <future>

class Device;
class Texture;
struct DecodedImage;
struct DecodedCubemap;

class TextureLoader {
public:
    // Automatically picks the correct loader based on file extension
    static std::unique_ptr<Texture> load(Device& device, const std::string& path);

    // Explicit loaders
    static std::unique_ptr<Texture> load2D(Device& device, const std::string& path, bool srgb = true);
    static std::unique_ptr<Texture> loadHDR(Device& device, const std::string& path);
    static std::unique_ptr<Texture> loadCubemap(Device& device, const std::string& directoryPath);

    // Async version (decode in background, upload on main thread)
    static std::future<std::unique_ptr<Texture>> loadAsync(Device& device, const std::string& path, bool srgb = true);

private:
    static std::unique_ptr<Texture> loadFromDecoded(Device& device, const DecodedImage& img, bool srgb);
    static std::unique_ptr<Texture> loadFromDecoded(Device& device, const DecodedCubemap& cube, bool srgb);
};

