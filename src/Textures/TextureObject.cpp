#include "TextureObject.h"
#include "../base/Device.h"

#include <stdexcept>
#include <cassert>

//// Public API ////

TextureObject::TextureObject(Device& device)
    : device(device) {
}


TextureObject::~TextureObject() {
    destroyResources();
}


// Manually constructed by TextureUploader
TextureObject::TextureObject(Device& device, const TextureInitInfo& info)
    : device(device),
    textureImage(info.image),
    textureImageMemory(info.memory),
    textureImageView(info.view),
    textureSampler(info.sampler),
    textureExtent{ info.width, info.height },
    mipLevel(info.mipLevels),
    createdArrayLayers(info.arrayLayers),
    createdImageFlags(info.flags),
    isLoaded(true),
    ownsImage(true){
}


//// Destructor helper ////

void TextureObject::destroyResources() {
    if (!isLoaded)
        return;

    VkDevice logicalDevice = device.device();

    if (textureSampler) {
        vkDestroySampler(logicalDevice, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }

    if (textureImageView) {
        vkDestroyImageView(logicalDevice, textureImageView, nullptr);
        textureImageView = VK_NULL_HANDLE;
    }

    if (textureImage) {
        vkDestroyImage(logicalDevice, textureImage, nullptr);
        textureImage = VK_NULL_HANDLE;
    }

    if (textureImageMemory) {
        vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        textureImageMemory = VK_NULL_HANDLE;
    }

    isLoaded = false;
}


//// Descriptor ////

VkDescriptorImageInfo TextureObject::getImageInfo() const {
    VkDescriptorImageInfo info{};
    info.sampler = textureSampler;
    info.imageView = textureImageView;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return info;
}


// Recreate view (used when switching between type of views)
void TextureObject::recreateImageView(bool isCubeMap, bool isHdr) {
    assert(textureImage != VK_NULL_HANDLE);

    // Destroy old view
    if (textureImageView) {
        vkDestroyImageView(device.device(), textureImageView, nullptr);
        textureImageView = VK_NULL_HANDLE;
    }

    VkFormat format = isHdr ?
        VK_FORMAT_R16G16B16A16_SFLOAT :
        VK_FORMAT_R8G8B8A8_UNORM;

    if (isCubeMap) {
        textureImageView = createCubeMapView(format);
    }
    else {
        textureImageView = createImageView(
            textureImage,
            format,
            mipLevel,
            VK_IMAGE_ASPECT_COLOR_BIT,
            false
        );
    }
}

void TextureObject::updateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode)
{
    if (textureSampler) {
        vkDestroySampler(device.device(), textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
	}

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = mipLevel;

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

//// Internal helpers ////

VkImageView TextureObject::createImageView(
    VkImage image,
    VkFormat format,
    uint32_t mipmapLevel,
    VkImageAspectFlagBits aspect,
    bool isCube
) const
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipmapLevel;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = isCube ? 6u : 1u;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("TextureObject: Failed to create image view");
    }

    return imageView;
}


VkImageView TextureObject::createCubeMapView(VkFormat format) const {
    return createImageView(
        textureImage,
        format,
        mipLevel,
        VK_IMAGE_ASPECT_COLOR_BIT,
        true // isCube
    );
}
