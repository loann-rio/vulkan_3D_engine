#pragma once

#include <vulkan/vulkan.h>
#include <memory>

class Device;

struct DecodedImage; 

class Texture {
public:
    Texture(Device& device);
    ~Texture();

    // Move only
    Texture(Texture&&) noexcept = delete;
    Texture& operator=(Texture&&) noexcept = delete;

    // non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Convenience getter
    VkDescriptorImageInfo getImageInfo() const;

    // Recreate view if format / type changed
    void recreateImageView(bool isCubeMap, bool isHdr);

    // Basic getters
    VkImage image() const { return textureImage; }
    VkImageView view() const { return textureImageView; }
    VkSampler sampler() const { return textureSampler; }
    uint32_t mipLevels() const { return mipLevel; }
    VkExtent2D extent() const { return textureExtent; }
    bool loaded() const { return isLoaded; }

private:
    // private constructor used by createFromGpu
    Texture(Device& device,
        VkImage image,
        VkDeviceMemory memory,
        VkImageView view,
        VkSampler sampler,
        uint32_t width, uint32_t height,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        VkImageCreateFlags flags);

    // helpers used by recreateImageView
    VkImageView createTextureCubeMapImageView(VkFormat format);
    VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipmapLevel, VkImageAspectFlagBits aspectFlag, bool isCubeMap);

private:
    friend class TextureUploader;

    void createImage();
    void allocateMemory();
    void createImageView();
    void createSampler();

private:
    Device& device;

    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

    VkExtent2D textureExtent{ 0,0 };
    uint32_t mipLevel = 1;
    uint32_t createdArrayLayers = 1;
    VkImageCreateFlags createdImageFlags = 0;

    bool isLoaded = false;
};