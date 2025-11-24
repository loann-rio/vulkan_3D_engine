#pragma once

#include <string>
#include <vector>
#include <memory>

struct DecodedImage {
    std::vector<unsigned char> pixels8;
    std::vector<float> pixels32;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool isFloat = false;
};

struct DecodedCubemap {
    // 6 faces in order: +X, -X, +Y, -Y, +Z, -Z
    std::vector<DecodedImage> faces;
};

class ImageDecoder {
public:
    virtual ~ImageDecoder() = default;

    virtual bool canDecode(const std::string& path) const = 0;
    virtual DecodedImage decode(const std::string& path) const = 0;
};
