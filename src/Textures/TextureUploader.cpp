#include "TextureUploader.h"

#include "Texture.h"

#include "Decoder/ImageDecoder.h"

#include "../base/Device.h"     
#include "../base/Buffer.h"

#include <stdexcept>
#include <cmath>
#include <vector>
#include <array>
#include <cstring>

//// helper functions ////

/// <summary>
/// Calculates the number of mipmap levels required for a texture of the given dimensions
/// </summary>
uint32_t TextureUploader::calculateMipLevels(int width, int height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

/// <summary>
/// Validates that the six cubemap faces have identical dimensions and the same pixel format
/// </summary>
/// <param name="faces">Array of six DecodedImage objects representing the cubemap faces</param>
void TextureUploader::validateCubemapFaces(
    const std::array<DecodedImage, 6>& faces)
{
    const int w = faces[0].width;
    const int h = faces[0].height;
    const bool isFloat = faces[0].isFloat;

    for (int i = 1; i < 6; i++) {
        if (faces[i].width != w ||
            faces[i].height != h)
        {
            throw std::runtime_error("All cubemap faces must have identical dimensions");
        }

        if (faces[i].isFloat != isFloat) {
            throw std::runtime_error("All cubemap faces must have the same pixel format");
        }
    }
}

/// <summary>
/// Validates a decoded image for correctness, format, and integrity
/// </summary>
/// <param name="img">The decoded image to validate</param>
void TextureUploader::validateImage(const DecodedImage& img)
{
    if (img.width <= 0 || img.height <= 0)
        throw std::runtime_error("Image dimensions must be positive");

    if (!img.isFloat && img.pixels8.empty())
        throw std::runtime_error("pixels8 data missing for 8-bit image");

    if (img.isFloat && img.pixels32.empty())
        throw std::runtime_error("pixels32 data missing for float image");
}


/// <summary>
/// Selects a Vulkan VkFormat for a texture based on if it uses float components and if it should be sRGB
/// </summary>
VkFormat TextureUploader::selectFormat(bool isFloat, bool srgb)
{
    if (isFloat) return VK_FORMAT_R32G32B32A32_SFLOAT;
    return srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
}


/// <summary>
/// Calculates the total pixel data size in bytes for a cubemap made of six faces
/// </summary>
/// <param name="faces">An array of six DecodedImage objects representing the cubemap faces</param>
/// <returns>The total size in bytes of the pixel data for all six faces</returns>
size_t TextureUploader::calculateCubemapPixelSize(
    const std::array<DecodedImage, 6>& faces)
{
    const bool isFloat = faces[0].isFloat;

    const size_t perFace = isFloat
        ? faces[0].pixels32.size() * sizeof(float)
        : faces[0].pixels8.size();

    return perFace * 6;
}

/// <summary>
/// Compute the size in bytes of the decoded image pixel data
/// </summary>
/// <returns>number of bytes required to hold the image pixel data</returns>
size_t TextureUploader::calculatePixelSize(const DecodedImage& img)
{
    return img.isFloat
        ? img.pixels32.size() * sizeof(float)
        : img.pixels8.size();
}


/// <summary>
/// Copies six cubemap face pixel data into staging memory
/// </summary>
/// <param name="faces">array of six DecodedImage objects representing the cubemap faces</param>
void TextureUploader::copyCubemapToMemory(
    Device& device,
    VkDeviceMemory stagingMemory,
    const std::array<DecodedImage, 6>& faces)
{
    uint8_t* dst = nullptr;
    vkMapMemory(device.device(), stagingMemory, 0, VK_WHOLE_SIZE, 0, (void**)&dst);

    size_t faceSize = calculateCubemapPixelSize({ faces }) / 6;

    for (uint32_t i = 0; i < 6; i++) {
        const void* src = faces[i].isFloat
            ? (const void*)faces[i].pixels32.data()
            : (const void*)faces[i].pixels8.data();

        std::memcpy(dst + i * faceSize, src, faceSize);
    }

    vkUnmapMemory(device.device(), stagingMemory);
}



/// <summary>
/// copies provided data into mapped Vulkan device memory
/// </summary>
/// <param name="device">Reference to the Device wrapper used to obtain the VkDevice for vkMapMemory/vkUnmapMemory operations</param>
/// <param name="memory">VkDeviceMemory handle that identifies the device memory to map and write to</param>
/// <param name="size">Number of bytes to copy into the mapped memory (the mapping range length)</param>
/// <param name="data">Pointer to the source buffer containing at least size bytes to be copied into device memory</param>
void TextureUploader::copyToMemory(Device& device, VkDeviceMemory memory, size_t size, const void* data) {
    void* mapped = nullptr;
    vkMapMemory(device.device(), memory, 0, size, 0, &mapped);
    std::memcpy(mapped, data, size);
    vkUnmapMemory(device.device(), memory);
}

/// <summary>
/// Creates a VkImage and allocates its device memory 
/// </summary>
VkImage TextureUploader::createImage(Device& device, 
    uint32_t width, uint32_t height,
    VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkDeviceMemory& imageMemory,
    uint32_t arrayLayer, VkImageCreateFlags flags, 
    VkImageType imageType, uint32_t mipLevels)
{
    assert(arrayLayer > 0 && "array layer cannot be zero");

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;

    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayer;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;

    imageInfo.flags = flags;

    VkImage image;
    device.createImageWithInfo(imageInfo, properties, image, imageMemory);

	return image;
}

/// <summary>
/// Creates a VkImageView for the specified Vulkan image
/// </summary>
/// <returns> VkImageView handle representing the created image view for the given image and parameters. The caller is responsible for destroying the view when no longer needed</returns>
VkImageView TextureUploader::createImageView(Device& device,
    VkImage image, VkFormat format, 
    uint32_t mipmapLevel, 
    VkImageAspectFlagBits aspectFlag, 
    VkImageViewType viewType, 
	uint32_t layerCount
)
{
    assert(mipmapLevel > 0 && "miplevel cannot be zero");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
	viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlag;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipmapLevel;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

    VkImageView imageView;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

/// <summary>
/// Creates and returns VkSampler configured for the given device and number of mipmap levels
/// </summary>
VkSampler TextureUploader::createSampler(Device& device, uint32_t mipLevels) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.minLod = 0.0f;
    info.maxLod = static_cast<float>(mipLevels);
    info.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
    info.anisotropyEnable = VK_TRUE;

    VkSampler sampler;
    if (vkCreateSampler(device.device(), &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler");

    return sampler;
}

//// MipMap generation ////

/// <summary>
/// Generates mipmaps for a Vulkan image by recording a single-time command buffer that blits from higher-resolution mip levels to lower ones and transitions image layouts as required
/// </summary>
/// <param name="device">Reference to the Device used to begin and end the single-time command buffer and to submit the recorded commands</param>
/// <param name="image">VkImage to generate mipmaps for</param>
/// <param name="mipLevels">number of mipmap levels to generate</param>
/// <param name="layers">Number of array layers in the image (defaults to 1)</param>
void TextureUploader::generateMipmaps(
    Device& device,
    VkImage image,
    int texWidth,
    int texHeight,
    uint32_t mipLevels,
    uint32_t layers = 1)
{
    VkCommandBuffer cmd = device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layers;
    barrier.subresourceRange.levelCount = 1;

    int mipW = texWidth;
    int mipH = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipW, mipH, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layers;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { std::max(1, mipW / 2), std::max(1, mipH / 2), 1 };
        blit.dstSubresource = blit.srcSubresource;
        blit.dstSubresource.mipLevel = i;

        vkCmdBlitImage(
            cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        // Previous mip now shader readable
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        mipW = std::max(1, mipW / 2);
        mipH = std::max(1, mipH / 2);
    }

    // Transition last mip level
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    device.endSingleTimeCommands(cmd);
}

//// Texture Uploading ////

std::unique_ptr<Texture> TextureUploader::upload2D(
    Device& device,
    const DecodedImage& img,
    bool srgb)
{
    const bool isFloat = img.isFloat;
    const uint32_t w = img.width;
    const uint32_t h = img.height;
    const uint32_t mipLevels = calculateMipLevels(w, h);

    const VkFormat format = selectFormat(isFloat, srgb);

	const size_t pixelSize = calculatePixelSize(img)    ;

    // staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(pixelSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    const void* srcPixels = isFloat ?
        (void*)img.pixels32.data() :
        (void*)img.pixels8.data();

    copyToMemory(device, stagingBufferMemory, pixelSize, srcPixels);

    // GPU image
    VkDeviceMemory imageMemory;
    VkImage image = createImage(
        device, w, h, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageMemory, 1, 0, VK_IMAGE_TYPE_2D, mipLevels
        );

    // transitions + copy
    device.transitionImageLayout(image, format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		mipLevels);

    device.copyBufferToImage(stagingBuffer, image, w, h, 1, 0);

    device.transitionImageLayout(image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        mipLevels);

    // cleanup staging
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

    // create view and sampler
    VkImageView   view    = createImageView(device, image, format, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
    VkSampler     sampler = createSampler(device, mipLevels);

    return std::make_unique<Texture>(
        device,
        image,
        imageMemory,
        view,
        sampler,
        w, h,
        mipLevels,
        format);
}

std::unique_ptr<Texture> TextureUploader::uploadCubemap(Device& device, const std::array<DecodedImage, 6>& faces)
{
    validateCubemapFaces(faces);

    const int width = faces[0].width;
    const int height = faces[0].height;
    const bool isFloat = faces[0].isFloat;
    const uint32_t mipLevels = calculateMipLevels(width, height);

    const VkFormat format = selectFormat(isFloat, /*srgb:*/ true);
    const size_t   totalSize = calculateCubemapPixelSize(faces);


	// staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device.createBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Copy all 6 images into one contiguous block
    copyCubemapToMemory(device, stagingBufferMemory, faces);

    VkDeviceMemory imageMemory;

    VkImage image = createImage(
        device, 
        width, height, 
        format, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        imageMemory, 
        6, 
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 
        VK_IMAGE_TYPE_2D, 
        mipLevels);

    // create copy regions
    const size_t faceSize =  calculateCubemapPixelSize(faces) / 6;
    std::vector<VkBufferImageCopy> bufferCopyRegions(6);

    for (uint32_t face = 0; face < 6; face++) {
        auto& r = bufferCopyRegions[face];
        r.bufferOffset = face * faceSize;
        r.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        r.imageSubresource.mipLevel = 0;
        r.imageSubresource.baseArrayLayer = face;
        r.imageSubresource.layerCount = 1;
        r.imageExtent = {
            (uint32_t)faces[0].width,
            (uint32_t)faces[0].height,
            1 };
    }

    device.transitionImageLayout(image, format,
        VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        mipLevels, 6);

    device.copyBufferToImage(stagingBuffer, image, bufferCopyRegions);

    device.transitionImageLayout(image, format, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
        mipLevels, 6);

	// cleanup staging
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
    
	// create view and sampler
    VkImageView   view    = createImageView(device, image, format, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);
    VkSampler     sampler = createSampler(device, mipLevels);

    return std::make_unique<Texture>(
        device,
        image,
        imageMemory,
        view,
        sampler,
        width, height,
        mipLevels,
		format);

}
