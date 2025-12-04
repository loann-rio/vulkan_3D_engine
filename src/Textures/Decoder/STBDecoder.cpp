#include "STBDecoder.h"

#include "STBDecoder.h"
#include "HDRDecoder.h"
#include "KTX2Decoder.h"
#include "KTXDecoder.h"

#include "../../external/stb/stb_image.h"

#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <filesystem>



namespace {

    static const std::array<const char*, 6> FACE_PATTERNS_POS = {
        "posx", "negx", "posy", "negy", "posz", "negz"
    };

    static const std::array<const char*, 6> FACE_PATTERNS_ALT = {
        "right", "left", "top", "bottom", "front", "back"
    };

    static const std::array<const char*, 6> KTX_CUBE_FACE_ORDER = {
        "+X", "-X", "+Y", "-Y", "+Z", "-Z"
    };

    /// <summary>
    /// List available decoders for regular 2D images(directory loading)
    /// </summary>
    /// <returns>usable decoder</returns>
    std::array<std::unique_ptr<ImageDecoder>, 4> createDecoders()
    {
        std::array<std::unique_ptr<ImageDecoder>, 4> decoders{
            std::make_unique<STBDecoder> (),
            std::make_unique<HDRDecoder> (),
            std::make_unique<KTX2Decoder>(),
            std::make_unique<KTXDecoder> ()
        };

        return decoders;
    }

    /// <summary>
	/// select and use the right decoder a 2D image
    /// </summary>
    DecodedImage decodeSingle2D(const std::string& path)
    {
        auto decoders = createDecoders();

        for (auto& dec : decoders)
        {
            if (dec->canDecode(path))
                return dec->decode(path);
        }

        throw std::runtime_error("No decoder found for: " + path);
    }
    
    /// <summary>
	/// simple getter to retrieve a list of files in a directory
    /// </summary>
    std::vector<std::filesystem::path> getFilesInDirectory(const std::string& directoryPath)
    {
        std::vector<std::filesystem::path> files;
        for (auto& entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (!entry.is_regular_file())
                continue;
            files.push_back(entry.path());
        }
        return files;
    }


    /// <summary>
    /// Searches a list of files for a set of cubemap face filenames using one of the provided filename patterns and returns the matched file paths in face order
    /// </summary>
    /// <param name="files">list of files to search in</param>
    /// <param name="patterns">vector of candidate patterns. Each element is an array of six C-style string substrings</param>
    /// <returns>full file paths as strings for the six cubemap faces, ordered according to the matched pattern</returns>
    static std::array<std::string, 6> findFaceFiles(
        const std::vector<std::filesystem::path>& files,
        std::vector<std::array<const char*, 6>> patterns)
    {
        std::array<std::string, 6> faceFiles;
        bool found = false;

        for (const auto& patt : patterns)
        {
            bool matchPattern = true;

            for (int i = 0; i < 6; i++)
            {
                auto it = std::find_if(files.begin(), files.end(),
                    [&](const std::filesystem::path& p) {
                        std::string lower = p.filename().string();
                        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                        return lower.find(patt[i]) != std::string::npos;
                    });

                if (it == files.end())
                {
                    matchPattern = false;
                    break;
                }
                faceFiles[i] = it->string();
            }

            if (matchPattern)
            {
                found = true;
                break;
            }
        }

        if (!found)
            throw std::runtime_error("Could not determine cubemap ordering from directory");

        return faceFiles;
    }

    /// <summary>
	/// valideates that the six cubemap faces have identical dimensions and the same pixel format
    /// </summary>
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

}

/// <summary>
/// Determines if the file extension is matching a format supported by the STB-based decoder
/// </summary>
/// <param name="path">path to extension that will be checked</param>
bool STBDecoder::canDecode(const std::string& path) const
{
    const std::string ext = imDecoder::getExtension(path);

    // STB-supported formats
    return (
        ext == "png" ||
        ext == "jpg" || ext == "jpeg" ||
        ext == "bmp" ||
        ext == "tga" ||
        ext == "gif" ||
        ext == "ppm" || ext == "pgm" || ext == "pnm");
}

/// <summary>
/// Loads an image from the given file path using stb_image and returns it as a DecodedImage with 8-bit RGBA pixels
/// </summary>
/// <param name="path">Filesystem path to the image file to load</param>
/// <returns>DecodedImage</returns>
DecodedImage STBDecoder::decode(const std::string& path) const
{
    DecodedImage img{};
    img.isFloat = false;

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!data) {
        throw std::runtime_error("STBDecoder: Failed to load image: " + path);
    }

    img.width = static_cast<uint32_t>(width);
    img.height = static_cast<uint32_t>(height);
    img.channels = 4;

    const size_t size = static_cast<size_t>(width) * height * 4;

    img.pixels8 = std::vector<unsigned char>(size);
    std::memcpy(img.pixels8.data(), data, size);

    stbi_image_free(data);
    return img;
}


/// <summary>
/// create cubemap from directory of 6 images corresponding to cubemap faces
/// </summary>
/// <param name="directoryPath">directory containing the 6 faces</param>
/// <returns>DecodedCubemap containing 6 DecodedImage</returns>
DecodedCubemap STBDecoder::decodeCubemapFromDirectory(const std::string& directoryPath) const
{
    if (!std::filesystem::exists(directoryPath))
        throw std::runtime_error("Cubemap directory does not exist: " + directoryPath);

    auto files = getFilesInDirectory(directoryPath);

    if (files.size() < 6)
        throw std::runtime_error("Cubemap directory must contain at least 6 images.");

    auto faceFiles = findFaceFiles(
        files,
        { FACE_PATTERNS_POS, FACE_PATTERNS_ALT }
    );

    // Decode the 6 faces
    DecodedCubemap cubemap;
    for (int i = 0; i < 6; i++)
        cubemap.faces[i] = decodeSingle2D(faceFiles[i]);

    validateCubemap(cubemap);
    return cubemap;
}
