#pragma once
#include <memory>

#include <vulkan/vulkan.h>

#include "TextureObject.h"

class Device;
//class TextureObject;
struct DecodedImage;
struct DecodedCubemap;

class TextureUploader {
public:
    // Upload a standard 2D image (8-bit or float)
    static std::unique_ptr<TextureObject> upload2D(
        Device& device,
        const DecodedImage& img,
        bool srgb
    );

    // Upload a cubemap (6 faces)
    static std::unique_ptr<TextureObject> uploadCubemap(
        Device& device,
        const DecodedCubemap& cubeMap
    );

    static std::unique_ptr<TextureObject> uploadCompressed2D(
        Device& device,
        const DecodedImage& imageData
    );

private:
    
   
    /// <summary>
    /// Creates a VkImage and allocates its device memory 
    /// </summary>
    static VkImage createImage(
        Device& device, 
        uint32_t width, uint32_t height, 
        VkFormat format, 
        VkImageTiling tiling, 
        VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VkDeviceMemory& imageMemory, 
        uint32_t arrayLayer, 
        VkImageCreateFlags flags, 
        VkImageType imageType, 
        uint32_t mipLevels
    );

    /// <summary>
    /// Creates a VkImageView for the specified Vulkan image
    /// </summary>
    /// <returns> VkImageView handle representing the created image view for the given image and parameters. The caller is responsible for destroying the view when no longer needed</returns>
    static VkImageView createImageView(
        Device& device, 
        VkImage image, 
        VkFormat format, 
        uint32_t mipmapLevel, 
        VkImageAspectFlagBits aspectFlag, 
        VkImageViewType viewType, 
        uint32_t layerCount
    );

    /// <summary>
    /// Creates and returns VkSampler configured for the given device and number of mipmap levels
    /// </summary>
    static VkSampler createSampler(
        Device& device, 
        uint32_t mipLevels
    );

    /// <summary>
    /// Generates mipmaps for a Vulkan image by recording a single-time command buffer that blits from higher-resolution mip levels to lower ones and transitions image layouts as required
    /// </summary>
    static void generateMipmaps(
        Device& device, 
        VkImage image, 
        int texWidth, int texHeight, 
        uint32_t mipLevels, uint32_t layers
    );
};
