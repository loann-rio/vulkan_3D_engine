#include "KTXDecoder.h"


#include <stdexcept>
#include <array>
#include <algorithm>
#include <filesystem>

#include <ktx.h>
#include <ktxvulkan.h>

namespace {

    void validateKtx2D(ktxTexture1* tex)
    {
        if (tex->numFaces != 1)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX decode error: file contains multiple faces (cubemap?)");
        }

        if (tex->numLayers > 1)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX decode error: array textures not supported in this loader");
        }

        // Must be a 2D image
        if (tex->numDimensions != 2)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX decode error: only 2D textures are supported");
        }
    }

    void validateKtxCubemap(ktxTexture1* tex)
    {
        if (!tex->isCubemap)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX file is not cubemap");
        }

        if (tex->numFaces != 6)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX texture is not a cubemap (numFaces != 6)");
        }

        if (tex->numLayers != 1)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX cubemap has invalid number of array layers (must be 1)");
        }
    }

    void validateCubemap(const DecodedCubemap& cube)
    {
        const auto& ref = cube.faces[0];

        for (int i = 1; i < 6; i++)
        {
            const auto& f = cube.faces[i];
            if (f.width != ref.width || f.height != ref.height)
                throw std::runtime_error("Cubemap faces must have identical dimensions.");

            if (f.format != ref.format)
                throw std::runtime_error("Cubemap faces must have identical VkFormat.");

            if (f.isCompressed != ref.isCompressed)
                throw std::runtime_error("Cubemap faces must have identical compression state.");

            if (f.mipLevels != ref.mipLevels)
                throw std::runtime_error("Cubemap faces must have same number of mip levels.");

            if (f.isFloat != ref.isFloat)
                throw std::runtime_error("Cubemap faces must be all-float or all-uint8.");
        }
    }

    /// <summary>
    /// Determine number of channels from VkFormat
    /// /// <summary>
    uint32_t channelsFromFormat(VkFormat f) {
        switch (f) {
        case VK_FORMAT_R8_UNORM: return 1;
        case VK_FORMAT_R8G8_UNORM: return 2;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8_UNORM: return 3;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB: return 4;
            // block-compressed: semantics may be 3 or 4; return 4 as a safe upper bound for channels reported
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return 4;
        default:
            return 4; // default conservative
        }
    }


    /// <summary>
	/// find defined texture format in ktxTexture, throws if undefined
    /// </summary>
    VkFormat findVkFormat(ktxTexture1* tex)
    {
        VkFormat format = ktxTexture1_GetVkFormat(tex);

        if (format == VK_FORMAT_UNDEFINED)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("KTX decode error: unsupported texture format");
        }

		return format;
	}

    /// <summary>
	/// get offset of specific image in ktxTexture
    /// </summary>
    ktx_size_t getKtxImageOffset(ktxTexture1* tex, uint32_t level, uint32_t layer, uint32_t face)
    {
        ktx_size_t offset;
        KTX_error_code err = ktxTexture_GetImageOffset(ktxTexture(tex), level, layer, face, &offset);
        if (err != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(tex));
            throw std::runtime_error("Failed to get KTX cubemap face offset.");
        }
        return offset;
    }
}

/// <summary>
/// Determines whether the provided file path has the KTX2 file extension
/// </summary>
/// <param name="path">file path or name to check</param>
bool KTXDecoder::canDecode(const std::string& path) const
{
    return imDecoder::getExtension(path) == "ktx";
}

DecodedImage KTXDecoder::decode(const std::string& path) const
{
    DecodedImage img{};

    // Load via KTX library
    ktxTexture1* texture = nullptr;
    KTX_error_code result = ktxTexture1_CreateFromNamedFile(
        path.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,// KTX_TEXTURE_CREATE_NO_FLAGS,
        &texture
    );

    if (result != KTX_SUCCESS || texture == nullptr) {
        throw std::runtime_error("KTX2Decoder: Failed to load texture: " + path);
    }

	validateKtx2D(texture);

    // Basic info
    img.width = texture->baseWidth;
    img.height = texture->baseHeight;
    img.mipLevels = texture->numLevels;
    img.isCompressed = true;
    img.format = findVkFormat(texture);


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

        ktx_size_t offset = getKtxImageOffset(texture, level, 0, 0);
        ktx_size_t size = ktxTexture_GetImageSize(ktxTexture(texture), level);

        img.mipOffsets[level] = static_cast<uint32_t>(offset);
        img.mipSizes[level] = static_cast<uint32_t>(size);
    }

    // Cleanup
    ktxTexture_Destroy(ktxTexture(texture));

    return img;
}

DecodedCubemap KTXDecoder::decodeCubemap(const std::string& path) const
{
    ktxTexture1* tex = nullptr;
    KTX_error_code result = ktxTexture1_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to load KTX file: " + path);

    validateKtxCubemap(tex);

    // KTX stores: arrayLayers=1, faces=6, mipLevels
    const uint32_t w = tex->baseWidth;
    const uint32_t h = tex->baseHeight;
    const uint32_t mipLevels = tex->numLevels;

    ktx_size_t totalSize = ktxTexture_GetDataSize(ktxTexture(tex));

    DecodedCubemap resultCube;
    for (uint32_t face = 0; face < 6; face++)
    {
        ktx_size_t offset = getKtxImageOffset(tex, 0, 0, face);

        DecodedImage& img = resultCube.faces[face];
        img.width = w;
        img.height = h;
        img.mipLevels = mipLevels;
        img.format = findVkFormat(tex);
		img.channels = channelsFromFormat(img.format);
        img.isCompressed = tex->isCompressed;
        img.isFloat = false;
        img.dataSize = static_cast<size_t>(totalSize);
        img.compressedData.resize(img.dataSize);

        memcpy(img.compressedData.data(), (uint8_t*)tex->kvData + offset, img.dataSize); 

        // Try to derive format
        
    }

    ktxTexture_Destroy(ktxTexture(tex));

    validateCubemap(resultCube);
    return resultCube;
}


