#pragma once
#include <memory>

#include <vulkan/vulkan.h>

class Device;
class Texture;
struct DecodedImage;
struct DecodedCubemap;

class TextureUploader {
public:
    // Upload a standard 2D image (8-bit or float)
    static std::unique_ptr<Texture> upload2D(
        Device& device,
        const DecodedImage& img,
        bool srgb
    );

    // Upload a cubemap (6 faces)
    static std::unique_ptr<Texture> uploadCubemap(
        Device& device,
        const std::array<DecodedImage, 6>& faces
    );

private:
    
    /// <summary>
    /// Calculates the number of mipmap levels required for a texture of the given dimensions
    /// </summary>
    static uint32_t calculateMipLevels(
        int width, int height
    );

    /// <summary>
    /// Validates that the six cubemap faces have identical dimensions and the same pixel format
    /// </summary>
    /// <param name="faces">Array of six DecodedImage objects representing the cubemap faces</param>
    static void validateCubemapFaces(
        const std::array<DecodedImage, 6>& faces
    );

    /// <summary>
    /// Validates a decoded image for correctness, format, and integrity
    /// </summary>
    /// <param name="img">The decoded image to validate</param>
    static void validateImage(
        const DecodedImage& img
    );

    /// <summary>
    /// Selects a Vulkan VkFormat for a texture based on if it uses float components and if it should be sRGB
    /// </summary>
    static VkFormat selectFormat(
        bool isFloat, bool srgb
    );

    /// <summary>
    /// Calculates the total pixel data size in bytes for a cubemap made of six faces
    /// </summary>
    /// <param name="faces">array of six DecodedImage objects representing the cubemap faces</param>
    /// <returns>The total size in bytes of the pixel data for all six faces</returns>
    static size_t calculateCubemapPixelSize(
        const std::array<DecodedImage, 6>& faces
    );

    /// <summary>
    /// Compute the size in bytes of the decoded image pixel data
    /// </summary>
    /// <returns>number of bytes required to hold the image pixel data</returns>
    static size_t calculatePixelSize(
        const DecodedImage& img
    );

    /// <summary>
    /// Copies six cubemap face pixel data into staging memory
    /// </summary>
    /// <param name="faces">array of six DecodedImage objects representing the cubemap faces</param>
    static void copyCubemapToMemory(
        Device& device, 
        VkDeviceMemory stagingMemory, 
        const std::array<DecodedImage, 6>& faces
    );

    /// <summary>
    /// copies provided data into mapped Vulkan device memory
    /// </summary>
    /// <param name="device">Reference to the Device wrapper used to obtain the VkDevice for vkMapMemory/vkUnmapMemory operations</param>
    /// <param name="memory">VkDeviceMemory handle that identifies the device memory to map and write to</param>
    /// <param name="size">Number of bytes to copy into the mapped memory (the mapping range length)</param>
    /// <param name="data">Pointer to the source buffer containing at least size bytes to be copied into device memory</param>
	static void copyToMemory(
        Device& device, 
        VkDeviceMemory memory, 
        size_t size, 
        const void* data
    );

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
