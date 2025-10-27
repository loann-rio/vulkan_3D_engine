#include "Texture.h"

#include "../base/Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../external/stb/stb_image.h"

#include <stdexcept>


std::unique_ptr<Texture> Texture::create(Device& device, const char* path, bool isCubeMap)
{
    auto tex = std::unique_ptr<Texture>(new Texture(device, path, isCubeMap));
    if (!tex->isLoaded) {
        return nullptr;
    }
    return tex;
}

std::unique_ptr<Texture> Texture::create(Device& device, std::vector<std::vector<glm::vec2>> imageArray)
{
    // Validate input
    if (imageArray.empty() || imageArray[0].empty()) {
        std::cerr << "Texture::create(imageArray): empty image array\n";
        return nullptr;
    }

    const uint32_t height = static_cast<uint32_t>(imageArray.size());
    const uint32_t width = static_cast<uint32_t>(imageArray[0].size());

    const uint32_t mipLevel = 1; // no mipmap generation here
    const VkDeviceSize imageSize = width * height * 4;

    // Allocate RGBA8 buffer
    unsigned char* rgba = new unsigned char[imageSize];

    size_t idx = 0;
    for (uint32_t y = 0; y < height; ++y) {
        const auto& row = imageArray[y];
        for (uint32_t x = 0; x < width; ++x) {
            const glm::vec2& c = row[x];
      
            rgba[idx + 0] = c.x;
            rgba[idx + 1] = c.y * 100;
            rgba[idx + 2] = 0;
            rgba[idx + 3] = 1;
            idx += 4;
        }
    }

    auto tex = std::unique_ptr<Texture>(new Texture(device, rgba, width, height, imageSize, mipLevel));

    delete[] rgba;

    if (!tex->isLoaded) {
        return nullptr;
    }

    return tex;
}

std::unique_ptr<Texture> Texture::createEmpty(Device& device, uint32_t width, uint32_t height, VkFormat format, bool isCubeMap)
{

    // usage can be :
    // classic texture
    // cube map
    // depth

    auto tex = std::unique_ptr<Texture>(new Texture(device, width, height, format, isCubeMap));
    if (!tex->isLoaded) {
        return nullptr;
    }
    return tex;
}

std::unique_ptr<Texture> Texture::createEmpty(Device& device, VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount)
{
    auto tex = std::unique_ptr<Texture>(new Texture(device, imageInfo, viewInfo, samplerInfo, initImageLayout, layerCount));
    if (!tex->isLoaded) {
        return nullptr;
    }
    return tex;
}



Texture::Texture(Device& device, const char* filePathTexture, bool isCubeMap) : device{device}
{
    const std::string path = filePathTexture;

	// Get file extension
	std::string extension;
    if (path.find_last_of(".") != std::string::npos) {
		extension = path.substr(path.find_last_of(".") + 1);
    }

    bool isHdr = stbi_is_hdr(filePathTexture);

    bool result = false;

	// Load texture according to file extension
    if (extension == "ktx") {
        result = createTextureImageKtx(path, isCubeMap);
    }
    else if (extension == "ktx2") {
        result = createTextureImageKtx2(path, isCubeMap);
    }
    else if (!isCubeMap) {
        result = createTextureImage(filePathTexture);
    }

	// Error check
    if (!result) {
        std::cerr << "could not load texture image \n";
        return;
    }

	// create image view and sampler
    textureImageView = createImageView(textureImage, (isHdr) ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_SRGB, mipLevel, VK_IMAGE_ASPECT_COLOR_BIT, isCubeMap);
    createTextureSampler(); 

    isLoaded = true;
}

Texture::Texture(Device& device, unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize, uint32_t mipLevel) : device{ device }
{
    createTextureImage(rgbaPixels, fontWidth, fontHeight, imageSize);
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, mipLevel, VK_IMAGE_ASPECT_COLOR_BIT, false);
    createTextureSampler();
    isLoaded = true;
}

Texture::Texture(Device& device, uint32_t width, uint32_t height, VkFormat format, bool isCubeMap) : device{ device }
{
    createImage(
        width,
        height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory);

    textureImageView = createImageView(textureImage, format, 1, VK_IMAGE_ASPECT_DEPTH_BIT, false);
    createTextureSampler();

    isLoaded = true;

}

Texture::Texture(Device& device, VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount) : device{ device }
{
    // create image
    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    // create image view
    viewInfo.image = textureImage;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    // create sampler
    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    if (initImageLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        device.transitionImageLayout(textureImage, imageInfo.format,
            VK_IMAGE_LAYOUT_UNDEFINED, initImageLayout, layerCount);

    isLoaded = true;
}

bool Texture::createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize) { 

    if (!rgbaPixels) {
        std::cerr << "failed to load texture image! \n";
        return false;
    }

    int texWidth = fontWidth;
    int texHeight = fontHeight;
    mipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

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
    //delete[] rgbaPixels;

    createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory);

    device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel);

    device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1, 0);

    if (mipLevel > 1)
        generateMipChain(textureImage, mipLevel, texWidth, texHeight);
    else
        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

	return true;
}

bool Texture::createTextureImage(float* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize)
{
    if (!rgbaPixels) {
        std::cerr << "failed to load texture image! \n";
        return false;
    }

    int texWidth = fontWidth;
    int texHeight = fontHeight;
    mipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (imageSize == 0)
        imageSize = texWidth * texHeight * 4 * sizeof(float);

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
    //delete[] rgbaPixels;

    createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory);


    device.transitionImageLayout(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel);

    device.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1, 0);

    if (mipLevel > 1)
        generateMipChain(textureImage, mipLevel, texWidth, texHeight);
    else
        device.transitionImageLayout(textureImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

    return true;
}



bool Texture::createTextureImage(const char* path)
{
    // create image from file
    int texWidth, texHeight, texChannels;
    bool result = false;

    if (stbi_is_hdr(path)) { /// if the image is hdr, we need to store the image in float and not char
        float* pixels = stbi_loadf(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        result = createTextureImage(pixels, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        //// free local memory
        stbi_image_free(pixels);
    }
    else
    {
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        result = createTextureImage(pixels, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        //// free local memory
        stbi_image_free(pixels);
    }

    return result;
}

bool Texture::createTextureImageKtx2(const std::string path, bool isCubeMap) 
{

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; 
    basist::ktx2_transcoder ktxTranscoder;

    basist::basisu_transcoder_init(); 

    std::ifstream ifs( path, std::ios::binary | std::ios::in | std::ios::ate );
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not load the requested image file " + path);
    }

    uint32_t inputDataSize = static_cast<uint32_t>(ifs.tellg());// get size 
    char* inputData = new char[inputDataSize]; // create local buffer

    ifs.seekg(0, std::ios::beg);
    ifs.read(inputData, inputDataSize); // write to buffer

    bool success = ktxTranscoder.init(inputData, inputDataSize);
    if (!success) {
        throw std::runtime_error("Could not initialize ktx2 transcoder for image file " + path);
    }

    // Select target format based on device features (use uncompressed if none supported)
    auto targetFormat = basist::transcoder_texture_format::cTFRGBA32;

    {
        VkPhysicalDeviceFeatures pFeatures;

        device.getPhysicalFeatures(&pFeatures);

        // select available format
        if (pFeatures.textureCompressionBC) {
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
        if (pFeatures.textureCompressionASTC_LDR) {
            if (device.isFormatSupported(VK_FORMAT_ASTC_4x4_SRGB_BLOCK))
            {
                targetFormat = basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
                format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
            }
        }

        // Ericsson texture compression
        if (pFeatures.textureCompressionETC2) {
            if (device.isFormatSupported(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK))
            {
                targetFormat = basist::transcoder_texture_format::cTFETC2_RGBA;
                format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
            }
        }
    }

    // @todo PowerVR texture compression support needs to be checked via an extension (VK_IMG_FORMAT_PVRTC_EXTENSION_NAME)
    const bool targetFormatIsUncompressed = basist::basis_transcoder_format_is_uncompressed(targetFormat);

    std::vector<basist::ktx2_image_level_info> levelInfos(ktxTranscoder.get_levels());
    mipLevel = ktxTranscoder.get_levels();

    // Query image level information that we need later on for several calculations
    // We only support 2D images (no cube maps or layered images)
    for (uint32_t i = 0; i < mipLevel; i++) {
        ktxTranscoder.get_image_level_info(levelInfos[i], i, 0, 0);
    }

    uint32_t width = levelInfos[0].m_orig_width;
    uint32_t height = levelInfos[0].m_orig_height;

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs{};

    // Create one staging buffer large enough to hold all uncompressed image levels
    const uint32_t bytesPerBlockOrPixel = basist::basis_get_bytes_per_block_or_pixel(targetFormat);
    uint32_t numBlocksOrPixels = 0;
    VkDeviceSize totalBufferSize = 0;

    for (uint32_t i = 0; i < mipLevel; i++) {
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
        throw std::runtime_error("Could not start transcoding for image file " + path);
    }

    // Transcode all mip levels into the staging buffer
    for (uint32_t i = 0; i < mipLevel; i++) {
        // Size calculations differ for compressed/uncompressed formats
        numBlocksOrPixels = targetFormatIsUncompressed ? levelInfos[i].m_orig_width * levelInfos[i].m_orig_height : levelInfos[i].m_total_blocks;
        uint32_t outputSize = numBlocksOrPixels * bytesPerBlockOrPixel;
        if (!ktxTranscoder.transcode_image_level(i, 0, 0, bufferPtr, numBlocksOrPixels, targetFormat, 0)) {
            throw std::runtime_error("Could not transcode the requested image file " + path);
        }
        bufferPtr += outputSize;
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(totalBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


    // transfer to device and copy from staging
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, totalBufferSize, 0, &data);
    memcpy(data, buffer, static_cast<size_t>(totalBufferSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    if (!isCubeMap) {
        createImage(
            width, height,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel);
        device.copyBufferToImage(stagingBuffer, textureImage, width, height, 1, mipLevel - 1);
        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel);

    }
    else
    {
        createImage(
            width, height,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory, 
            6,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT); 



        std::vector<VkBufferImageCopy> bufferCopyRegions;
        bufferCopyRegions.reserve(6 * mipLevel);

        uint32_t offset = 0;

        for (uint32_t face = 0; face < 6; face++)
        {
            for (uint32_t level = 0; level < mipLevel; level++) 
            {
                auto& levelInfo = levelInfos[level]; 

                uint32_t width = std::max(1u, levelInfo.m_orig_width); 
                uint32_t height = std::max(1u, levelInfo.m_orig_height); 
                uint32_t blocksOrPixels = targetFormatIsUncompressed ? width * height : levelInfo.m_total_blocks; 
                uint32_t imageSize = blocksOrPixels * bytesPerBlockOrPixel; 

                if (!ktxTranscoder.transcode_image_level(level, face, 0, buffer + offset, blocksOrPixels, targetFormat, 0)) {
                    throw std::runtime_error("Transcoding failed for level " + std::to_string(level) + ", face " + std::to_string(face));
                } 
                    
                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent = { width, height, 1 };
                    
                bufferCopyRegion.bufferOffset = offset;

                bufferCopyRegions.push_back(bufferCopyRegion);

                offset += imageSize; 
            }
        }

        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, 6); 
        device.copyBufferToImage(stagingBuffer, textureImage, bufferCopyRegions);
        device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel, 6);

    }

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

    return true;

}

bool Texture::createTextureImageKtx(const std::string path, bool isCubeMap)
{
    
    assert(!path.empty() && "Texture path is empty");

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    // open save file
    std::ifstream ifs(path, std::ios::binary | std::ios::in | std::ios::ate);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not load the requested image file " + path);
    }

    uint32_t inputDataSize = static_cast<uint32_t>(ifs.tellg());// get size 
    char* inputData = new char[inputDataSize]; // create local buffer

    ifs.seekg(0, std::ios::beg);
    ifs.read(inputData, inputDataSize); // write to buffer



    // init transcoder
    ktxTexture* kTexture;
    KTX_error_code result = ktxTexture_CreateFromMemory(
        reinterpret_cast<const ktx_uint8_t*>(inputData),
        inputDataSize,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &kTexture
    );

    if (result != KTX_SUCCESS) {
        throw std::runtime_error("Failed to load KTX texture: " + std::to_string(result));
    }

    uint32_t width = kTexture->baseWidth;
    uint32_t height = kTexture->baseHeight;
    mipLevel = kTexture->numLevels;

    assert(width > 0 && height > 0 && "Invalid texture size");


    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(kTexture);
    ktx_size_t ktxTextureSize = ktxTexture_GetElementSize(kTexture);
    ktx_size_t totalSize = ktxTexture_GetDataSize(kTexture);

    uint32_t faceCount = kTexture->numFaces;
    uint32_t layerCount = kTexture->numLayers;

    uint32_t arrayLayers = isCubeMap ? (faceCount * layerCount) : layerCount;

    createImage(
        width, height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureImage,
        textureImageMemory,
        arrayLayers,
        isCubeMap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


    // transfer to device and copy from staging
    void* data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, totalSize, 0, &data);
    memcpy(data, ktxTextureData, static_cast<size_t>(totalSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    for (uint32_t layer = 0; layer < layerCount; ++layer) {
        for (uint32_t face = 0; face < faceCount; ++face) {
            for (uint32_t level = 0; level < mipLevel; ++level) {
                ktx_size_t imgOffset = 0;
                KTX_error_code rc = ktxTexture_GetImageOffset(kTexture, level, layer, face, &imgOffset);
                assert(rc == KTX_SUCCESS);

                ktx_size_t imgSize = ktxTexture_GetImageSize(kTexture, level);
                assert(imgSize > 0 && "Invalid image size");

                uint32_t mipW = std::max(1u, width >> level);
                uint32_t mipH = std::max(1u, height >> level);

                VkBufferImageCopy region{};
                region.bufferOffset = static_cast<VkDeviceSize>(imgOffset);
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = level;
                // baseArrayLayer must index layers of the Vulkan image.
                // If arrayLayers = faceCount * layerCount, then layout is typically:
                // for layerIndex in [0..layerCount-1]:
                //   arrayLayer = layerIndex*faceCount + face
                region.imageSubresource.baseArrayLayer = layer * faceCount + face;
                region.imageSubresource.layerCount = 1;
                region.imageExtent = { mipW, mipH, 1 };
                bufferCopyRegions.push_back(region);
            }
        }
    }

    device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, layerCount * faceCount);
    device.copyBufferToImage(stagingBuffer, textureImage, bufferCopyRegions);
    device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevel, layerCount * faceCount);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);

    ktxTexture_Destroy(kTexture);
    delete[] inputData;

    return true;
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

void Texture::createTextureSampler()
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
    samplerInfo.maxLod = mipLevel;

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

}

void Texture::createImage(uint32_t width, uint32_t height,
    VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
    VkImage& image, VkDeviceMemory& imageMemory, 
    uint32_t arrayLayer, VkImageCreateFlags flags)
{

    assert(arrayLayer > 0 && "array layer cannot be zero");

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    
    imageInfo.mipLevels = mipLevel;
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
       

    if (vkCreateImage(device.device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    bind(image, properties, imageMemory);
}

void Texture::generateMipChain(VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height)
{
    // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    int32_t mipWidth = width;
    int32_t mipHeight = height;


    for (uint32_t i = 1; i < mipLevels; i++) 
    {
       

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i - 1;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;
        mipSubRange.baseArrayLayer = 0;
        

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = mipSubRange;

            vkCmdPipelineBarrier(
                commandBuffer, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0, 
                0, nullptr, 
                0, nullptr, 
                1, &imageMemoryBarrier
            );
        }

        VkImageBlit imageBlit{};

        imageBlit.srcOffsets[0] = { 0, 0, 0 };
        imageBlit.srcOffsets[1] = { int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1 };

        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcSubresource.baseArrayLayer = 0;

        imageBlit.dstOffsets[0] = { 0, 0, 0 };
        imageBlit.dstOffsets[1] = { int32_t(std::max(int32_t(1), int32_t(width >> i))), int32_t(std::max(int32_t(1), int32_t(height >> i))), 1 };

        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstSubresource.baseArrayLayer = 0;

        vkCmdBlitImage(
            commandBuffer, 
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, &imageBlit, 
            VK_FILTER_LINEAR
        );

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = mipSubRange;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                0, 
                0, nullptr, 
                0, nullptr, 
                1, &imageMemoryBarrier
            );
        }
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.layerCount = 1;
    subresourceRange.baseMipLevel = mipLevels - 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.levelCount = 1;

    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        vkCmdPipelineBarrier(
            commandBuffer, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
            0, 
            0, nullptr, 
            0, nullptr, 
            1, &imageMemoryBarrier
        );
    }

    device.endSingleTimeCommands(commandBuffer);
}

VkDescriptorImageInfo Texture::getImageInfo()
{
    return { textureSampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
}

void Texture::setSampler(VkSamplerCreateInfo samplerInfo) 
{
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevel);

    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

VkImageView Texture::createTextureCubeMapImageView() 
{
    assert(mipLevel > 0 && "miplevel cannot be zero");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
    viewInfo.image = textureImage; 
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; 
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB; 
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevel; 
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    VkImageView imageView; 
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) { 
        throw std::runtime_error("failed to create texture image view!"); 
    }

    return imageView;  
}

VkImageView Texture::createImageView(VkImage image, VkFormat format, bool isCubeMap)
{
    assert(mipLevel > 0 && "miplevel cannot be zero"); 

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCubeMap? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevel;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = isCubeMap ? 6 : 1;

    VkImageView imageView;
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

