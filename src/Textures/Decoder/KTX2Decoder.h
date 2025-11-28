#pragma once
#include "ImageDecoder.h"

#include <ktx.h>


struct CopyEntry { ktx_size_t offset; ktx_size_t size; uint32_t faceIndex; }; 

class KTX2Decoder : public ImageDecoder {
public:

    /// <summary>
    /// Determines whether the provided file path has the KTX2 file extension
    /// </summary>
    /// <param name="path">file path or name to check</param>
    /// <returns>true if the extracted extension is exactly "ktx2"</returns>
    bool canDecode(
        const std::string& path
    ) const override;

    /// <summary>
    /// Loads a KTX2 texture file using the KTX library
    /// </summary>
    /// <param name="path">Path to the KTX2 file to decode</param>
    /// <returns>DecodedImage with metadata </returns>
    DecodedImage decode(
        const std::string& path
    ) const override;

    /// <summary>
	/// Loads a KTX2 texture file using the KTX library into cubemap format
    /// </summary>
    /// <param name="path">Path to the KTX2 file to decode</param>
    /// <returns>DecodedImage with metadata </returns>
    DecodedCubemap decodeCubemap(
        const std::string& filePath
	) const;
};
