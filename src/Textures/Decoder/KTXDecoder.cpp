#include "KTXDecoder.h"


#include <stdexcept>
#include <array>
#include <algorithm>
#include <filesystem>

#include <ktx.h>
#include <ktxvulkan.h>

namespace {

    void validateKtx2D(ktxTexture* tex)
    {
        if (tex->numFaces != 1)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX decode error: file contains multiple faces (cubemap?)");
        }

        if (tex->numLayers > 1)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX decode error: array textures not supported in this loader");
        }

        // Must be a 2D image
        if (tex->numDimensions != 2)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX decode error: only 2D textures are supported");
        }
    }

    void validateKtxCubemap(ktxTexture* tex)
    {
        if (!tex->isCubemap)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX file is not cubemap");
        }

        if (tex->numFaces != 6)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX texture is not a cubemap (numFaces != 6)");
        }

        if (tex->numLayers != 1)
        {
            ktxTexture_Destroy(tex);
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
	/// find defined texture format in ktxTexture, throws if undefined
    /// </summary>
    VkFormat findVkFormat(ktxTexture* tex)
    {
        VkFormat format = ktxTexture_GetVkFormat(tex);

        if (format == VK_FORMAT_UNDEFINED)
        {
            ktxTexture_Destroy(tex);
            throw std::runtime_error("KTX decode error: unsupported texture format");
        }
	}

    /// <summary>
	/// get offset of specific image in ktxTexture
    /// </summary>
    ktx_size_t getKtxImageOffset(ktxTexture* tex, uint32_t level, uint32_t layer, uint32_t face)
    {
        ktx_size_t offset;
        KTX_error_code err = ktxTexture_GetImageOffset(tex, level, layer, face, &offset);
        if (err != KTX_SUCCESS)
        {
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
    ktxTexture* tex = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to load KTX file: " + path);

	validateKtx2D(tex);

	const VkFormat format = findVkFormat(tex);

    const uint32_t w = tex->baseWidth;
    const uint32_t h = tex->baseHeight;
    const uint32_t mipLevels = tex->numLevels;
    const ktx_size_t imageSize = ktxTexture_GetImageSize(tex, 0);

	ktx_size_t offset = getKtxImageOffset(tex, 0, 0, 0);

    const uint8_t* src = tex->pData + offset;

    DecodedImage img;
    img.width = w;
    img.height = h;
    img.format = format;
    img.mipLevels = mipLevels;
    img.dataSize = imageSize;
    img.isCompressed = tex->isCompressed;

    img.isFloat = (
        format == VK_FORMAT_R16G16B16A16_SFLOAT ||
        format == VK_FORMAT_R32G32B32A32_SFLOAT ||
        format == VK_FORMAT_R32G32B32_SFLOAT);

    if (img.isCompressed)
    {
        img.compressedData.assign(src, src + imageSize);
    }
    else if (img.isFloat)
    {
        img.pixels32.resize(imageSize / sizeof(float));
        std::memcpy(img.pixels32.data(), src, imageSize);
    }
    else
    {
        img.pixels8.assign(src, src + imageSize);
    }

    ktxTexture_Destroy(tex);
    return img;
}

DecodedCubemap KTXDecoder::decodeCubemap(const std::string& path) const
{
    ktxTexture* tex = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &tex);
    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to load KTX file: " + path);

    validateKtxCubemap(tex);

    // KTX stores: arrayLayers=1, faces=6, mipLevels
    const uint32_t w = tex->baseWidth;
    const uint32_t h = tex->baseHeight;
    const uint32_t mipLevels = tex->numLevels;

    DecodedCubemap resultCube;
    for (uint32_t face = 0; face < 6; face++)
    {
        ktx_size_t offset = getKtxImageOffset(tex, 0, 0, face);

        DecodedImage& img = resultCube.faces[face];
        img.width = w;
        img.height = h;
        img.mipLevels = mipLevels;
        img.channels = 4; // KTX does not tell channels; format determines this
        img.isCompressed = tex->isCompressed;
        img.isFloat = false;
        img.dataSize = ktxTexture_GetImageSize(tex, 0);
        img.compressedData.resize(img.dataSize);

        memcpy(img.compressedData.data(), tex->kvData + offset, img.dataSize);

        // Try to derive format
        img.format = ktxTexture_GetVkFormat(tex);
        if (img.format == VK_FORMAT_UNDEFINED)
        {
            // vkFormat is not set
            throw std::runtime_error("KTX file missing VkFormat");
        }
    }

    ktxTexture_Destroy(tex);

    validateCubemap(resultCube);
    return resultCube;
}


