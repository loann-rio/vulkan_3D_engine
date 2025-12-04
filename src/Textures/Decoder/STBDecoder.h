#pragma once
#include "ImageDecoder.h"

class STBDecoder final : public ImageDecoder {
public:

    /// <summary>
    /// Determines if the file extension is matching a format supported by the STB-based decoder
    /// </summary>
    /// <param name="path">Path whose extension will be checked. The function extracts the extension via getExtension and compares it against supported formats</param>
    /// <returns>true if the extracted extension is one of the supported formats (png, jpg, jpeg, bmp, tga, gif, ppm, pgm, pnm); false otherwise</returns>
    bool canDecode(
        const std::filesystem::path& path
    ) const override;

    /// <summary>
    /// Loads an image from the given file path using stb_image and returns it as a DecodedImage with 8-bit RGBA pixels
    /// </summary>
    /// <param name="path">Filesystem path to the image file to load</param>
    /// <returns>DecodedImage</returns>
    DecodedImage decode(
        const std::filesystem::path& path
    ) const override;

    /// <summary>
    /// create cubemap from directory of 6 images corresponding to cubemap faces
    /// </summary>
    /// <param name="directoryPath">directory containing the 6 faces</param>
    /// <returns>DecodedCubemap containing 6 DecodedImage</returns>
    DecodedCubemap decodeCubemapFromDirectory(
        const std::string& directoryPath
	) const;
};
