#include "KTX2Decoder.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace {

    /// <summary>
    /// Maps the vkFormat field of a ktxTexture2 to a corresponding VkFormat
    /// </summary>
    /// <param name="tex">Pointer to a ktxTexture2 whose vkFormat value will be mapped to a Vulkan VkFormat.</param>
    /// <returns>The matching VkFormat for the texture's vkFormat when it is supported.</returns>
    VkFormat mapKtxFormat(ktxTexture2* tex) {
        switch (tex->vkFormat) {
        case VK_FORMAT_BC7_SRGB_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return static_cast<VkFormat>(tex->vkFormat);

        default:
            break;
        }

        throw std::runtime_error("KTX2Decoder: Unsupported vkFormat: " + std::to_string(tex->vkFormat));
    }

    /// <summary>
	/// check if a ktxTexture2 is valid as a cubemap
    /// </summary>
    void checkCubemapCompatibility(ktxTexture2* kTexture) {
        
		// Check isCubemap flag
        if (!ktxTexture(kTexture)->isCubemap) {
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("KTX2Decoder::decodeCubemap - file is not a cubemap");
        }     

        // KTX stores faces, levels, layers; we expect 6 faces
        if (kTexture->numFaces != 6) {
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("KTX2Decoder::decodeCubemap - cubemap must have 6 faces");
        }
    }

    /// <summary>
	/// check if the resulting cubemap faces are compatible
    /// </summary>
    void validateResultCubemap(DecodedCubemap outCube) {

        const DecodedImage& ref = outCube.faces[0];

        for (int i = 1; i < 6; ++i) {

            const DecodedImage& f = outCube.faces[i];

            if (f.width != ref.width || f.height != ref.height)
                throw std::runtime_error("KTX2Decoder::decodeCubemap - cubemap faces mismatch");

            if (f.mipLevels != ref.mipLevels)
                throw std::runtime_error("KTX2Decoder::decodeCubemap - mip level mismatch");

            if (f.isCompressed != ref.isCompressed)
                throw std::runtime_error("KTX2Decoder::decodeCubemap - compression mismatch");
        }
	}

    /// <summary>
	/// calculate image offset for given level/layer/face
    /// </summary>
    ktx_size_t calculateImageOffset(ktxTexture2* kTexture, const uint32_t level, const uint32_t layer, const uint32_t face) {
        ktx_size_t offset = 0;
        KTX_error_code e = ktxTexture_GetImageOffset(ktxTexture(kTexture), level, layer, face, &offset);
        if (e != KTX_SUCCESS) {
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("KTX2Decoder::decodeCubemap - failed to get image offset.");
        }

		return offset;
    }

    /// <summary>
	/// calculate image size for given level
    /// </summary>
    ktx_size_t calculateImageSize(ktxTexture2* kTexture, const uint32_t level) {
        ktx_size_t size = ktxTexture_GetImageSize(ktxTexture(kTexture), level);
        if (size == 0) {
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("KTX2Decoder::decodeCubemap - invalid image size.");
        }
        return size;
	}

    /// <summary>
	/// gather image copy entries for all levels/layers/faces
    /// </summary>
    std::vector<CopyEntry> gatherImageCopyEntries(ktxTexture2* kTexture, const uint32_t levels, const uint32_t layers)
    {
        std::vector<CopyEntry> entries;

        entries.reserve(levels * std::max<uint32_t>(1, layers) * 6);

        for (uint32_t level = 0; level < levels; ++level)
        for (uint32_t layer = 0; layer < std::max<uint32_t>(1, layers); ++layer)
        for (uint32_t face = 0; face < 6; ++face)
        {
            ktx_size_t offset = calculateImageOffset(kTexture, level, layer, face);
            ktx_size_t size = calculateImageSize(kTexture, level);
            entries.push_back({ offset, size, face });
        }

        return entries;
	}

} // namespace

/// <summary>
/// Determines whether the provided file path has the KTX2 file extension
/// </summary>
/// <param name="path">file path to check</param>
bool KTX2Decoder::canDecode(const std::string& path) const {
    return imDecoder::getExtension(path) == "ktx2";
}

/// <summary>
/// Loads a KTX2 texture file using the KTX library and returns a DecodedImage containing its metadata and compressed data
/// </summary>
/// <param name="path">Path to the KTX2 file to decode</param>
/// <returns>DecodedImage with metadata </returns>
DecodedImage KTX2Decoder::decode(const std::string& path) const {
    DecodedImage img{};
    img.isCompressed = true;

    // Load via KTX library
    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        path.c_str(),
        KTX_TEXTURE_CREATE_NO_FLAGS,
        &texture
    );

    if (result != KTX_SUCCESS || texture == nullptr) {
        throw std::runtime_error("KTX2Decoder: Failed to load texture: " + path);
    }

    // Basic info
    img.width = texture->baseWidth;
    img.height = texture->baseHeight;
    img.mipLevels = texture->numLevels;
    img.format = mapKtxFormat(texture);

    // Extract compressed data for the full texture (all mipmaps)
    // ktxTexture_GetData returns a pointer to the raw block of texture data
    ktx_size_t totalSize = ktxTexture_GetDataSize(ktxTexture(texture));

	img.dataSize = static_cast<size_t>(totalSize);

    const uint8_t* data = reinterpret_cast<const uint8_t*>(ktxTexture_GetData(ktxTexture(texture)));

    if (!data || totalSize == 0) {
        ktxTexture_Destroy(ktxTexture(texture));
        throw std::runtime_error("KTX2Decoder: Empty or invalid compressed data.");
    }

    img.compressedData.resize(totalSize);
    std::memcpy(img.compressedData.data(), data, totalSize);

    // Cleanup
    ktxTexture_Destroy(ktxTexture(texture));

    return img;
}

DecodedCubemap KTX2Decoder::decodeCubemap(const std::string& filePath) const
{
    // Open texture (load image data)
    ktxTexture2* kTexture = nullptr;
    KTX_error_code rc = ktxTexture2_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
    if (rc != KTX_SUCCESS || !kTexture) {
        throw std::runtime_error("KTX2Decoder::decodeCubemap - failed to open file: " + filePath);
    }

	// Validate cubemap compatibility
	checkCubemapCompatibility(kTexture);

    // base dims & mips
    const uint32_t baseW = static_cast<uint32_t>(kTexture->baseWidth);
    const uint32_t baseH = static_cast<uint32_t>(kTexture->baseHeight);
    const uint32_t levels = static_cast<uint32_t>(kTexture->numLevels);
    const uint32_t layers = static_cast<uint32_t>(kTexture->numLayers ? kTexture->numLayers : 1);

    // Decide how to split: KTX2 cubemaps may be stored as arrayLayers=6 with numFaces=1,
    // or numFaces=6 and arrayLayers=1 depending on creation
    DecodedCubemap outCube;

    // For each face, we will append all mip levels' data for that face into compressedData
    for (uint32_t face = 0; face < 6; ++face) {
        DecodedImage& faceImg = outCube.faces[face];
        faceImg.width = baseW;
        faceImg.height = baseH;
        faceImg.mipLevels = levels;
        faceImg.isCompressed = true;
        faceImg.dataSize = 0;

        // try to copy format
        if (kTexture->vkFormat != 0) faceImg.format = static_cast<VkFormat>(kTexture->vkFormat);
        else faceImg.format = VK_FORMAT_UNDEFINED;
    }

	std::vector<CopyEntry> entries = gatherImageCopyEntries(kTexture, levels, layers);

    // Now compute total size per face and fill compressedData
    for (const auto& e : entries) {
        DecodedImage& faceImg = outCube.faces[e.faceIndex];
        faceImg.dataSize += static_cast<size_t>(e.size);
    }

    for (uint32_t face = 0; face < 6; ++face) {
        if (outCube.faces[face].dataSize > 0)
            outCube.faces[face].compressedData.resize(outCube.faces[face].dataSize);
    }

    // Copy bytes from kTexture->data into each face buffer
    for (const auto& e : entries) {
        // find where to append next chunk for that face
        DecodedImage& faceImg = outCube.faces[e.faceIndex];

        size_t currentOffset = faceImg.compressedData.size() - faceImg.dataSize; // head pointer
        // Find first zero position to append: we track remaining size in dataSize, so compute append position
        size_t used = 0;
        // compute used bytes by subtracting remaining dataSize from total allocated
        used = faceImg.compressedData.size() - faceImg.dataSize;
        // copy
        std::memcpy(faceImg.compressedData.data() + used, reinterpret_cast<const uint8_t*>(ktxTexture_GetData(ktxTexture(kTexture))) + e.offset, static_cast<size_t>(e.size));
        // reduce remaining dataSize
        faceImg.dataSize -= static_cast<size_t>(e.size);
    }

    // After copying we set dataSize back to actual sizes (compute from compressedData size)
    for (uint32_t face = 0; face < 6; ++face)
        outCube.faces[face].dataSize = outCube.faces[face].compressedData.size();

    ktxTexture_Destroy(ktxTexture(kTexture));

	validateResultCubemap(outCube);

    return outCube;
}
