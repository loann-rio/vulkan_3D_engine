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
        return static_cast<VkFormat>(tex->vkFormat);
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

        if (entries.empty()) {
            throw std::runtime_error("KTX2Decoder::decodeCubemap - no image entries found");
        }

        return entries;
	}

    ktxTexture2* getKtxTextureFromfile(const std::string& path)
    {
        ktxTexture2* texture = nullptr;
        KTX_error_code result = ktxTexture2_CreateFromNamedFile(
            path.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,// KTX_TEXTURE_CREATE_NO_FLAGS,
            &texture
        );

        if (result != KTX_SUCCESS || texture == nullptr) {
            throw std::runtime_error("KTX2Decoder: Failed to load texture: " + path);
        }

        return texture;
    }

    VkFormat transcodeBasisFormat(ktxTexture2* texture)
    {
        if (!ktxTexture2_NeedsTranscoding(texture)) {
            throw std::runtime_error("KTX2 has no vkFormat and no Basis data — cannot decode ");
        }

        // Transcode using Basis Universal inside KTX2
        KTX_error_code tc = ktxTexture2_TranscodeBasis(
            texture,
            KTX_TTF_BC7_RGBA,     // Desktop default
            KTX_TF_HIGH_QUALITY   // optional
        );

        if (tc != KTX_SUCCESS) {
            throw std::runtime_error("KTX2 transcoding failed: " + std::string(ktxErrorString(tc)));
        }

        return VK_FORMAT_BC7_UNORM_BLOCK;
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
    ktxTexture2* texture = getKtxTextureFromfile(path);

    // Basic info
    img.width = texture->baseWidth;
    img.height = texture->baseHeight;
    img.mipLevels = texture->numLevels;
    

    // If the KTX2 contains Basis supercompressed data, we must transcode it first
    if (ktxTexture2_NeedsTranscoding(texture)) {
        KTX_error_code status = ktxTexture2_TranscodeBasis(
            texture,
            KTX_TTF_BC7_RGBA,   // Choose your GPU format
            0                   // flags
        );
        if (status != KTX_SUCCESS) {
            ktxTexture_Destroy(ktxTexture(texture));
            throw std::runtime_error("KTX2Decoder: Transcoding failed.");
        }
    }

    img.format = mapKtxFormat(texture);


    // Extract compressed data for the full texture (all mipmaps)
    // ktxTexture_GetData returns a pointer to the raw block of texture data
    ktx_size_t totalSize = ktxTexture_GetDataSize(ktxTexture(texture));
	img.dataSize = static_cast<size_t>(totalSize);

    
    //const uint8_t* data = reinterpret_cast<const uint8_t*>(ktxTexture_GetData(ktxTexture(texture)));
    uint8_t* data = (uint8_t*)texture->pData;
    if (!data || totalSize == 0) {
        ktxTexture_Destroy(ktxTexture(texture));
        throw std::runtime_error("KTX2Decoder: Empty or invalid compressed data.");
    }

    img.compressedData.resize(totalSize);
    std::memcpy(img.compressedData.data(), data, totalSize);

    
	// get mipmaps sizes and offsets

    img.mipOffsets.resize(img.mipLevels);
    img.mipSizes.resize(img.mipLevels);

    for (uint32_t level = 0; level < img.mipLevels; level++) {
        
		ktx_size_t offset = calculateImageOffset(texture, level, 0, 0);
        ktx_size_t size = ktxTexture_GetImageSize(ktxTexture(texture), level);

        img.mipOffsets[level] = static_cast<uint32_t>(offset);
        img.mipSizes[level] = static_cast<uint32_t>(size);
    }

    // Cleanup
    ktxTexture_Destroy(ktxTexture(texture));

    return img;
}

DecodedCubemap KTX2Decoder::decodeCubemap(const std::string& filePath) const
{
    // load image data
    ktxTexture2* kTexture = getKtxTextureFromfile(filePath);

	// Validate cubemap compatibility
	checkCubemapCompatibility(kTexture);

    // base dims & mips
    const uint32_t baseW   = static_cast<uint32_t>(kTexture->baseWidth);
    const uint32_t baseH   = static_cast<uint32_t>(kTexture->baseHeight);
    const uint32_t levels  = static_cast<uint32_t>(kTexture->numLevels);
    const uint32_t layers  = static_cast<uint32_t>(kTexture->numLayers ? kTexture->numLayers : 1);
          VkFormat format  = static_cast<VkFormat>(kTexture->vkFormat);
    
    if (format == VK_FORMAT_UNDEFINED)
        format = transcodeBasisFormat(kTexture);

    DecodedCubemap outCube;

    // initialize faces (always 6 for a cubemap)
    for (uint32_t face = 0; face < 6; ++face) {
        DecodedImage& faceImg = outCube.faces[face];

        faceImg.width = baseW;
        faceImg.height = baseH;
        faceImg.mipLevels = levels;
        faceImg.isCompressed = true;
        faceImg.format = format;
    }

	std::vector<CopyEntry> entries = gatherImageCopyEntries(kTexture, levels, layers);
    for (const auto& e : entries) {
        DecodedImage& faceImg = outCube.faces[e.faceIndex];
        faceImg.compressedData.resize(faceImg.compressedData.size() + e.size);
    }


    const uint8_t* srcBase = reinterpret_cast<const uint8_t*>(ktxTexture_GetData(ktxTexture(kTexture)));
    if (srcBase == nullptr) {
        throw std::runtime_error("KTX2Decoder::decodeCubemap - ktx texture data pointer is null for: " + filePath);
    }

    std::array<size_t, 6> writeOffsets = { 0, 0, 0, 0, 0, 0 };

    for (const auto& e : entries) {
        DecodedImage& faceImg = outCube.faces[e.faceIndex];
        size_t dstOffset = writeOffsets[e.faceIndex];

        // Bounds check before copying
        if (dstOffset + static_cast<size_t>(e.size) > faceImg.compressedData.size()) {
            throw std::runtime_error("KTX2Decoder::decodeCubemap - write would overflow face buffer (file: " + filePath + ")");
        }

        std::memcpy(faceImg.compressedData.data() + dstOffset, srcBase + e.offset, static_cast<size_t>(e.size));

        //faceImg.dataSize = static_cast<size_t>(e.size);
        faceImg.mipOffsets.push_back(static_cast<uint32_t>(dstOffset));
        faceImg.mipSizes.push_back(static_cast<uint32_t>(e.size));

        writeOffsets[e.faceIndex] += static_cast<size_t>(e.size);
    }


    for (uint32_t f = 0; f < 6; ++f) {
        if (writeOffsets[f] != outCube.faces[f].compressedData.size()) {
            throw std::runtime_error("KTX2Decoder::decodeCubemap - written size mismatch for face " + std::to_string(f));
        }
        outCube.faces[f].dataSize = outCube.faces[f].compressedData.size();
    }

    ktxTexture_Destroy(ktxTexture(kTexture));

	validateResultCubemap(outCube);

    return outCube;
}
