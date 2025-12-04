#pragma once
#include "ImageDecoder.h"

#include <ktx.h>
#include <ktxvulkan.h>


class KTXDecoder : public ImageDecoder {

public:

    /// <summary>
    /// Determines whether the provided file path has the KTX file extension
    /// </summary>
    /// <param name="path">file path or name to check</param>
    bool canDecode(
        const std::string& path
    ) const override;

    /// <summary>
    /// Loads a KTX texture file into a DecodedImage struct
    /// </summary>
    /// <param name="path">Path to the KTX file to decode</param>
    /// <returns>DecodedImage with metadata </returns>
    DecodedImage decode(
        const std::string& path
    ) const override;

    /// <summary>
    /// Loads a KTX cubemap texture file into a DecodedCubemap struct
    /// </summary>
    /// <param name="path">Path to the KTX file to decode</param>
    /// <returns>DecodedImage with metadata </returns>
    DecodedCubemap decodeCubemap(
        const std::string& path
    ) const;
};
