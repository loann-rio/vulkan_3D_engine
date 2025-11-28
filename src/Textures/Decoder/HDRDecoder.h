#pragma once
#include "ImageDecoder.h"

class HDRDecoder : public ImageDecoder {
public:
    
    /// <summary>
    /// Checks whether this decoder can decode the file at the specified path by testing if its extension is "hdr"
    /// </summary>
    /// <param name="path">Path to the file to check; its extension is examined</param>
    /// <returns>true if the file extension equals "hdr", otherwise false</returns>
    bool canDecode(
        const std::string& path
    ) const override;

    /// <summary>
    /// Loads an image from the given file path using stb_image and returns it as a DecodedImage with 32-bit RGBA pixels
    /// </summary>
    /// <param name="path">Filesystem path to the image file to load</param>
    /// <returns>DecodedImage</returns>
    DecodedImage decode(
        const std::string& path
    ) const override;
};
