#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

class IVertexLayout {
public:
    virtual ~IVertexLayout() = default;
    virtual uint32_t stride() const = 0;
    virtual uint32_t attributeCount() const = 0;

    // attribute semantic description 
    struct Attribute {
        std::string name;
        VkFormat format;
        uint32_t offset;
        uint32_t size; // in bytes
    };

    virtual const std::vector<Attribute>& attributes() const = 0;
};