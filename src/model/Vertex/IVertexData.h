#pragma once
#include <cstdint>
#include <vector>
#include <memory>

#include "IVertexLayout.h"

// polymorphic vertex container
class IVertexData {
public:
    virtual ~IVertexData() = default;

    // layout describing the memory
    virtual const IVertexLayout& layout() const = 0;

    // count of vertices
    virtual uint32_t vertexCount() const = 0;

    // pointer to raw interleaved bytes
    virtual const void* rawData() const = 0;

    // stride in bytes
    virtual uint32_t stride() const = 0;
};


