#include "Texture.h"

#include <stdexcept>

#include "Buffer.h"
#include "basisu_transcoder.h"



#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(Device& device, const char* filePathTexture) : device{device}
{
    createTextureImage(filePathTexture);
    createTextureImageView();
    createTextureSampler();
}

Texture::Texture(Device& device, unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize, uint32_t mipLevel) : device{ device }
{ 
    createTextureImage(rgbaPixels, fontWidth, fontHeight, imageSize, mipLevel);
    createTextureImageView();
    createTextureSampler();
}

void Texture::createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize, uint32_t mipLevel) {

    int texWidth = fontWidth;
    int texHeight = fontHeight;

    if (imageSize == 0)
        imageSize = texWidth * texHeight * 4;

    // create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // transfer to device and copy from staging
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);

    memcpy(data, rgbaPixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    // free local memory
    delete[] rgbaPixels;

    createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory,
        mipLevel);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}

/*void Texture::createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize, uint32_t mipLevel, const bool targetFormatIsUncompressed) {
    /*
    int texWidth = fontWidth;
    int texHeight = fontHeight;



    if (imageSize == 0)
        imageSize = texWidth * texHeight * 4;

    // Create one staging buffer large enough to hold all uncompressed image levels
    const uint32_t bytesPerBlockOrPixel = basist::basis_get_bytes_per_block_or_pixel(targetFormat);
    uint32_t numBlocksOrPixels = 0;
    VkDeviceSize totalBufferSize = 0;

    for (uint32_t i = 0; i < mipLevel; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        totalBufferSize += numBlocksOrPixels * bytesPerBlockOrPixel;
    }

    for (uint32_t i = 0; i < mipLevel; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;
        if (!ktxTranscoder.transcode_image_level(i, 0, 0, bufferPtr, numBlocksOrPixels, targetFormat, 0)) {
            throw std::runtime_error("Could not transcode the requested image file " + filename);
        }
        bufferPtr += outputSize;
    }

    // create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // transfer to device and copy from staging
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);

    memcpy(data, rgbaPixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    // free local memory
    delete[] rgbaPixels;

    createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory,
        mipLevel);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkDeviceSize bufferOffset = 0;

    for (uint32_t i = 0; i < mipLevel; i++) {

        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;

        device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1, i, bufferOffset);

        bufferOffset += outputSize;


    }


    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

    basist::ktx2_transcoder ktxTranscoder;
    const std::string filename = path + "\\" + gltfimage.uri;
    std::ifstream ifs(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not load the requested image file " + filename);
    }

    uint32_t inputDataSize = static_cast<uint32_t>(ifs.tellg());// get size 
    char* inputData = new char[inputDataSize]; // create buffer

    ifs.seekg(0, std::ios::beg);
    ifs.read(inputData, inputDataSize); // write to buffer

    bool success = ktxTranscoder.init(inputData, inputDataSize);
    if (!success) {
        throw std::runtime_error("Could not initialize ktx2 transcoder for image file " + filename);
    }

    // Select target format based on device features (use uncompressed if none supported)
    auto targetFormat = basist::transcoder_texture_format::cTFRGBA32;

    VkPhysicalDeviceFeatures* pFeatures;
    device.getPhysicalFeatures(pFeatures);

    // select available format
    if (pFeatures->textureCompressionBC) {
        // BC7 is the preferred block compression if available
        if (device.isFormatSupported(VK_FORMAT_BC7_UNORM_BLOCK)) {
            targetFormat = basist::transcoder_texture_format::cTFBC7_RGBA;
            format = VK_FORMAT_BC7_UNORM_BLOCK;
        }
        else {
            if (device.isFormatSupported(VK_FORMAT_BC3_SRGB_BLOCK)) {
                targetFormat = basist::transcoder_texture_format::cTFBC3_RGBA;
                format = VK_FORMAT_BC3_SRGB_BLOCK;
            }
        }
    }

    // Adaptive scalable texture compression
    if (pFeatures->textureCompressionASTC_LDR) {
        if (device.isFormatSupported(VK_FORMAT_ASTC_4x4_SRGB_BLOCK))
        {
            targetFormat = basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
            format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        }
    }

    // Ericsson texture compression
    if (pFeatures->textureCompressionETC2) {
        if (device.isFormatSupported(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK))
        {
            targetFormat = basist::transcoder_texture_format::cTFETC2_RGBA;
            format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        }
    }

    delete pFeatures;

    // @todo PowerVR texture compression support needs to be checked via an extension (VK_IMG_FORMAT_PVRTC_EXTENSION_NAME)
    const bool targetFormatIsUncompressed = basist::basis_transcoder_format_is_uncompressed(targetFormat);

    std::vector<basist::ktx2_image_level_info> levelInfos(ktxTranscoder.get_levels());
    mipLevels = ktxTranscoder.get_levels();

    // Query image level information that we need later on for several calculations
    // We only support 2D images (no cube maps or layered images)
    for (uint32_t i = 0; i < mipLevels; i++) {
        ktxTranscoder.get_image_level_info(levelInfos[i], i, 0, 0);
    }

    width = levelInfos[0].m_orig_width;
    height = levelInfos[0].m_orig_height;


    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs{};



    // Create one staging buffer large enough to hold all uncompressed image levels
    const uint32_t bytesPerBlockOrPixel = basist::basis_get_bytes_per_block_or_pixel(targetFormat);
    uint32_t numBlocksOrPixels = 0;
    VkDeviceSize totalBufferSize = 0;

    for (uint32_t i = 0; i < mipLevels; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        totalBufferSize += numBlocksOrPixels * bytesPerBlockOrPixel;
    }

    Buffer stagingBufferMapped{ device, totalBufferSize, numBlocksOrPixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0 };


    unsigned char* buffer = new unsigned char[totalBufferSize];
    unsigned char* bufferPtr = &buffer[0];

    success = ktxTranscoder.start_transcoding();
    if (!success) {
        throw std::runtime_error("Could not start transcoding for image file " + filename);
    }

    // Transcode all mip levels into the staging buffer
    for (uint32_t i = 0; i < mipLevels; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;
        if (!ktxTranscoder.transcode_image_level(i, 0, 0, bufferPtr, numBlocksOrPixels, targetFormat, 0)) {
            throw std::runtime_error("Could not transcode the requested image file " + filename);
        }
        bufferPtr += outputSize;
    }

    memcpy(stagingBufferMapped.getMappedMemory(), buffer, totalBufferSize);


    Texture image{};
    image.createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), image.textureImage, image.textureImageMemory, mipLevels);
    image.createTextureImage();

    // Transcode and copy all image levels
    VkDeviceSize bufferOffset = 0;
    for (uint32_t i = 0; i < mipLevels; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;

        Device::copyBufferToImage(****, image, levelInfos[i].m_orig_width, levelInfos[i].m_orig_height, 1, i, bufferOffset);

        bufferOffset += outputSize;
    }

    Texture::transitionImageLayout(image, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipLevels);

    delete[] buffer;
    delete[] inputData;
}*/

void Texture::createTextureImage(const char* path)
{
    // create image from file
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(imageSize*2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // transfer to device and copy from staging
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    //// free local memory
    stbi_image_free(pixels);


    createImage(
        texWidth,
        texHeight, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        textureImage, 
        textureImageMemory, 
        mipLevels);

    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
    generateMipChain(mipLevels, texWidth, texHeight);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

}


void Texture::bind(VkImage& image, VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory)
{
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.device(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device.device(), image, imageMemory, 0);
}

void Texture::createTextureSampler(uint32_t mipLevel)
{

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float) mipLevel;

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

}

void Texture::createImage(uint32_t width, uint32_t height,
    VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
    VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels) 
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device.device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    bind(image, properties, imageMemory);
}

void Texture::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevel)
{
    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevel;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    device.endSingleTimeCommands(commandBuffer);
}

void Texture::generateMipChain(uint32_t mipLevels, uint32_t width, uint32_t height)
{

    /*VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device.getPhysicalDevice(), , &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }*/

    // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    for (uint32_t i = 1; i < mipLevels; i++) {
        VkImageBlit imageBlit{};

        //imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
        //imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
        //imageBlit.srcOffsets[1].z = 1;

        /*imageBlit.dstOffsets[1].x = int32_t(width >> i);
        imageBlit.dstOffsets[1].y = int32_t(height >> i);
        imageBlit.dstOffsets[1].z = 1;*/

        imageBlit.srcOffsets[0] = { 0, 0, 0 };
        imageBlit.srcOffsets[1] = { int32_t(width >> (i - 1)), (int) height, 0 };

        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcSubresource.baseArrayLayer = 0;

        imageBlit.dstOffsets[0] = { 0, 0, 0 };
        imageBlit.dstOffsets[1] = { int32_t(width >> i), int32_t(height >> i), 1 };
        
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstSubresource.baseArrayLayer = 0;

        

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i - 1;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.image = textureImage;
            imageMemoryBarrier.subresourceRange = mipSubRange;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        vkCmdBlitImage(commandBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = textureImage;
            imageMemoryBarrier.subresourceRange = mipSubRange;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount = 1;
    subresourceRange.levelCount = mipLevels;

    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.image = textureImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    device.endSingleTimeCommands(commandBuffer);
}

VkDescriptorImageInfo Texture::getImageInfo()
{
    return { textureSampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
}

void Texture::createTextureImageView(uint32_t mipLevel)
{
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, mipLevel);
}


VkImageView Texture::createImageView(VkImage image, VkFormat format, uint32_t mipLevel)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevel;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

