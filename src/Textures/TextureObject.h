#pragma once

#include <vulkan/vulkan.h>

class Device;

class TextureObject {
private:
    struct TextureInitInfo {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
        uint32_t width, height;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        VkImageCreateFlags flags;
    };

public:
    explicit TextureObject(Device& device);
    ~TextureObject();

    TextureObject(TextureObject&&) noexcept = delete;
    TextureObject& operator=(TextureObject&&) noexcept = delete;
    TextureObject(const TextureObject&) = delete;
    TextureObject& operator=(const TextureObject&) = delete;

    // descriptor helper for writes
    VkDescriptorImageInfo getImageInfo() const;

    // when the texture type changes
    void recreateImageView(bool isCubeMap, bool isHdr);

	void updateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode);

    // Getters
    VkImage image() const { return textureImage; }
    VkImageView view() const { return textureImageView; }
    VkSampler sampler() const { return textureSampler; }
    uint32_t mipLevels() const { return mipLevel; }
    VkExtent2D extent() const { return textureExtent; }
    bool loaded() const { return isLoaded; }

private:
    // private constructor used only by TextureUploader
	TextureObject(Device&, const TextureInitInfo& info);

    // helpers called by recreateImageView()
    VkImageView createImageView(VkImage image,
        VkFormat format,
        uint32_t mipmapLevel,
        VkImageAspectFlagBits aspect,
        bool isCube) const;

    VkImageView createCubeMapView(VkFormat format) const;

    void destroyResources();
private:

    Device& device;

    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

    VkExtent2D textureExtent{ 0, 0 };
    uint32_t mipLevel = 1;
    uint32_t createdArrayLayers = 1;
    VkImageCreateFlags createdImageFlags = 0;

    bool isLoaded = false;

    bool ownsImage = false;


    friend class TextureUploader;
    friend class TextureBuilder;
};
