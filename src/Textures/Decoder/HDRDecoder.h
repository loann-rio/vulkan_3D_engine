#pragma once
#include "ImageDecoder.h"

class HDRDecoder : public ImageDecoder {
public:
    
    /// <summary>
    ///  check is the decoder can decode the given file
    /// </summary>
    bool canDecode(
        const std::string& path
    ) const override;

    /// <summary>
    /// Loads an image from the given file path using stb_image and returns it as a DecodedImage with 32-bit RGBA pixels
    /// </summary>
    /// <param name="path">path to the image file to load</param>
    /// <returns>DecodedImage</returns>
    DecodedImage decode(
        const std::string& path
    ) const override;
};
