#pragma once
#include "ImageDecoder.h"

class CubemapDecoder {
public:
    // Load from directory containing 6 files
    static DecodedCubemap decodeFromDirectory(const std::string& directoryPath);

    // Future: decode from cross image
    // static DecodedCubemap decodeFromCross(const std::string& path);
};
