#pragma once
#include "ImageDecoder.h"

class STBDecoder : public ImageDecoder {
public:
    bool canDecode(const std::string& path) const override;
    DecodedImage decode(const std::string& path) const override;
};
