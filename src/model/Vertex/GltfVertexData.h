#pragma once

#include "IVertexData.h"
#include "IVertexLayout.h"

#include <glm/glm.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <vector>


struct GltfVertex {
    glm::vec3 position{};
    glm::vec3 normal{};

    glm::uvec4 joint0;
    glm::vec4 weight0;

    glm::vec2 uv0{};
    glm::vec2 uv1{};

    glm::vec3 color{1.f, 1.f, 1.f};


    bool operator==(const GltfVertex& other) const noexcept {
        return position == other.position &&
            normal == other.normal &&
            uv0 == other.uv0 &&
            uv1 == other.uv1 &&
            joint0 == other.joint0 &&
            weight0 == other.weight0 &&
            color == other.color;
    }
};

class GltfVertexLayout : public IVertexLayout {
public:
    GltfVertexLayout() {
        m_attrs.clear();

        m_attrs.push_back({ "position", offsetof(GltfVertex, position) , sizeof(GltfVertex::position) });
        m_attrs.push_back({ "color",    offsetof(GltfVertex, color)    , sizeof(GltfVertex::color)    });
        m_attrs.push_back({ "normal",   offsetof(GltfVertex, normal)   , sizeof(GltfVertex::normal)   });
        m_attrs.push_back({ "uv0",      offsetof(GltfVertex, uv0)      , sizeof(GltfVertex::uv0)      });
        m_attrs.push_back({ "uv1",      offsetof(GltfVertex, uv1)      , sizeof(GltfVertex::uv1)      });
        m_attrs.push_back({ "joint0",   offsetof(GltfVertex, joint0)   , sizeof(GltfVertex::joint0)   });
        m_attrs.push_back({ "weight0",  offsetof(GltfVertex, weight0)  , sizeof(GltfVertex::weight0)  });

        m_stride = static_cast<uint32_t>(sizeof(GltfVertex));
    };

    uint32_t stride() const override { return m_stride; }
    uint32_t attributeCount() const override { return static_cast<uint32_t>(m_attrs.size()); }
    const std::vector<IVertexLayout::Attribute>& attributes() const override { return m_attrs; }

private:
    std::vector<IVertexLayout::Attribute> m_attrs;
    uint32_t m_stride;
};


class GltfVertexData : public IVertexData {
public:
    GltfVertexData(std::vector<GltfVertex> verts) : m_vertices(std::move(verts)) {};

    const IVertexLayout& layout() const override { return s_layout; };
    uint32_t vertexCount() const override { return static_cast<uint32_t>(m_vertices.size()); };
    const void* rawData() const override { return m_vertices.data(); };
    uint32_t stride() const override { return s_layout.stride(); };

    const std::vector<GltfVertex>& cpuData() const { return m_vertices; }

private:
    std::vector<GltfVertex> m_vertices;
    static GltfVertexLayout s_layout;
};