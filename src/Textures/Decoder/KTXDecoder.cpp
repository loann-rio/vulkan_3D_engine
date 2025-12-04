#include "KTXDecoder.h"


#include <stdexcept>
#include <array>
#include <algorithm>
#include <filesystem>

struct CopyEntry { ktx_size_t offset; ktx_size_t size; uint32_t faceIndex; };


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
    /// check if a ktxTexture1 is valid as a cubemap
    /// </summary>
    void checkCubemapCompatibility(ktxTexture1* kTexture) {

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
    /// calculate image offset for given level/layer/face
    /// </summary>
    ktx_size_t calculateImageOffset(ktxTexture1* kTexture, const uint32_t level, const uint32_t layer, const uint32_t face) {
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
    ktx_size_t calculateImageSize(ktxTexture1* kTexture, const uint32_t level) {
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
    std::vector<CopyEntry> gatherImageCopyEntries(ktxTexture1* kTexture, const uint32_t levels, const uint32_t layers)
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


    ktxTexture1* getKtxTextureFromfile(const std::string& path)
    {
        ktxTexture1* texture = nullptr;
        KTX_error_code result = ktxTexture1_CreateFromNamedFile(
            path.c_str(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,// KTX_TEXTURE_CREATE_NO_FLAGS,
            &texture
        );

        if (result != KTX_SUCCESS || texture == nullptr) {
            throw std::runtime_error("KTX2Decoder: Failed to load texture: " + path);
        }

        return texture;
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

DecodedCubemap KTXDecoder::decodeCubemap(const std::string& filePath) const
{
    // load image data
    ktxTexture1* kTexture = getKtxTextureFromfile(filePath);

    // Validate cubemap compatibility
    checkCubemapCompatibility(kTexture);

    // base dims & mips
    const uint32_t baseW = static_cast<uint32_t>(kTexture->baseWidth);
    const uint32_t baseH = static_cast<uint32_t>(kTexture->baseHeight);
    const uint32_t levels = static_cast<uint32_t>(kTexture->numLevels);
    const uint32_t layers = static_cast<uint32_t>(kTexture->numLayers ? kTexture->numLayers : 1);
    VkFormat format = static_cast<VkFormat>(ktxTexture1_GetVkFormat(kTexture));

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
        throw std::runtime_error("KTXDecoder::decodeCubemap - ktx texture data pointer is null for: " + filePath);
    }

    std::array<size_t, 6> writeOffsets = { 0, 0, 0, 0, 0, 0 };

    for (const auto& e : entries) {
        DecodedImage& faceImg = outCube.faces[e.faceIndex];
        size_t dstOffset = writeOffsets[e.faceIndex];

        // Bounds check before copying
        if (dstOffset + static_cast<size_t>(e.size) > faceImg.compressedData.size()) {
            throw std::runtime_error("KTXDecoder::decodeCubemap - write would overflow face buffer (file: " + filePath + ")");
        }

        std::memcpy(faceImg.compressedData.data() + dstOffset, srcBase + e.offset, static_cast<size_t>(e.size));

        //faceImg.dataSize = static_cast<size_t>(e.size);
        faceImg.mipOffsets.push_back(static_cast<uint32_t>(dstOffset));
        faceImg.mipSizes.push_back(static_cast<uint32_t>(e.size));

        writeOffsets[e.faceIndex] += static_cast<size_t>(e.size);
    }


    for (uint32_t f = 0; f < 6; ++f) {
        if (writeOffsets[f] != outCube.faces[f].compressedData.size()) {
            throw std::runtime_error("KTXDecoder::decodeCubemap - written size mismatch for face " + std::to_string(f));
        }
        outCube.faces[f].dataSize = outCube.faces[f].compressedData.size();
    }

    ktxTexture_Destroy(ktxTexture(kTexture));

    validateResultCubemap(outCube);

    return outCube;
}
