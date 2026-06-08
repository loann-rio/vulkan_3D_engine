#include "PassTarget.h"

PassTarget::PassTarget(Device& device_, Swap_chain& swapchain_, AssetManager& assets_, VkExtent2D extent_, bool hasDepth, bool hasColor, size_t imageCount, bool isFinal) : device(device_), extent(extent_), assets(assets_) {

    if (hasColor)
        createColorTargetTexture(imageCount, swapchain_.getSwapChainImageFormat(), (isFinal) ? swapchain_.getSwapChainImages() : std::vector<VkImage>{});

    if (hasDepth)
        createDepthTargetTexture(imageCount, swapchain_.getSwapChainDepthFormat());

    createImageInfo();
}

PassTarget::~PassTarget()
{
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    }
}

void PassTarget::resizeTargets(VkExtent2D newExtent, uint32_t imageCount, VkFormat format, VkFormat depthFormat)
{
    extent = newExtent;
    cleanupTargetTextures();
    cleanupLocalFramebuffers();

    createColorTargetTexture(imageCount, format);
    createDepthTargetTexture(imageCount, depthFormat);

    createImageInfo();
}

void PassTarget::createLocalFramebuffers(VkRenderPass renderPass)
{
    size_t imageCount = std::max(color.size(), depth.size());
    assert(imageCount != 0 && "image count cannot be 0");

    framebuffers.resize(imageCount);
    for (size_t i = 0; i < framebuffers.size(); i++)
    {
        std::vector<VkImageView> attachments;
        if (color.size())
            attachments.push_back(assets.textures().get(color[i])->view());

        if (depth.size())
            attachments.push_back(assets.textures().get(depth[i])->view());

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device.device(),
            &framebufferInfo,
            nullptr,
            &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void PassTarget::createDescriptorSets(DescriptorPool& pool)
{
    auto textureSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    colorDescriptorSets.resize(color.size());
    for (int i = 0; i < colorDescriptorSets.size(); i++)
    {
        auto imageInfo = assets.textures().get(color[i])->getImageInfo();
        DescriptorWriter(*textureSetLayout, pool)
            .writeImage(0, &imageInfo)
            .build(colorDescriptorSets[i]);
    }

    depthDescriptorSets.resize(depth.size());
    for (int i = 0; i < depthDescriptorSets.size(); i++)
    {
        auto imageInfo = assets.textures().get(depth[i])->getImageInfo();
        DescriptorWriter(*textureSetLayout, pool)
            .writeImage(0, &imageInfo)
            .build(depthDescriptorSets[i]);
    }

}

void PassTarget::cleanupLocalFramebuffers()
{
    for (VkFramebuffer fb : framebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device.device(), fb, nullptr);
        }
    }

    framebuffers.clear();
}

void PassTarget::cleanupTargetTextures()
{
    // clean color textures
    for (auto texId : color) {
        if (texId != TextureManager::InvalidTextureID) {
            assets.textures().remove(texId);
        }
    }

    // clean depth textures
    for (auto texId : depth) {
        if (texId != TextureManager::InvalidTextureID) {
            assets.textures().remove(texId);
        }
    }
}

void PassTarget::createColorTargetTexture(uint32_t imageCount, VkFormat format, std::vector<VkImage> swapImage)
{
    color.resize(imageCount);

    for (int i = 0; i < imageCount; i++)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { extent.width, extent.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        TextureBuilder builder(device);

        if (swapImage.size())
        {
            color[i] = assets.textures().create(
                builder.fromTextureInfo(
                    swapImage[i],
                    { extent.width, extent.height },
                    viewInfo,
                    samplerInfo,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    1, format)
            );
        }
        else
        {
            color[i] = assets.textures().create(
                builder.fromTextureInfo(
                    imageInfo, viewInfo, samplerInfo,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

                )
            );
        }
    }
}

void PassTarget::createDepthTargetTexture(uint32_t imageCount, VkFormat format)
{
    depth.resize(imageCount);

    for (int i = 0; i < imageCount; i++)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { extent.width, extent.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;
        imageInfo.pNext = NULL;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 100.0f;

        TextureBuilder builder(device);
        depth[i] = assets.textures().create(
            builder.fromTextureInfo(
                imageInfo, viewInfo,
                samplerInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            ));
    }
}

void PassTarget::createImageInfo()
{
    colorImageInfo.resize(color.size());
    for (uint16_t i = 0; i < color.size(); i++)
        colorImageInfo[i] = assets.textures().get(color[i])->getImageInfo();

    depthImageInfo.resize(depth.size());
    for (uint16_t i = 0; i < depth.size(); i++)
        depthImageInfo[i] = assets.textures().get(depth[i])->getImageInfo();
}
