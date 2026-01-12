#pragma once

#include "IVertexData.h"
#include "IVertexLayout.h"

#include <glm/glm.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <vector>


struct ObjVertex {
    glm::vec3 position{};
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    glm::vec3 normal{};
    glm::vec2 uv{};
   
   
    bool operator==(const ObjVertex& other) const noexcept {
        return position == other.position &&
            normal == other.normal &&
            uv == other.uv &&
            color == other.color;
    }
};

class ObjVertexLayout : public IVertexLayout {
public:
    ObjVertexLayout() {
        m_attrs.clear();

        m_attrs.push_back({ "position", VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ObjVertex, position), sizeof(ObjVertex::position) });
        m_attrs.push_back({ "color",    VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ObjVertex, color)   , sizeof(ObjVertex::color) });
        m_attrs.push_back({ "normal",   VK_FORMAT_R32G32B32_SFLOAT,  offsetof(ObjVertex, normal)  , sizeof(ObjVertex::normal) });
        m_attrs.push_back({ "uv",       VK_FORMAT_R32G32_SFLOAT,     offsetof(ObjVertex, uv)      , sizeof(ObjVertex::uv) });

        m_stride = static_cast<uint32_t>(sizeof(ObjVertex));
    };

    uint32_t stride() const override { return m_stride; }
    uint32_t attributeCount() const override { return static_cast<uint32_t>(m_attrs.size()); }
    const std::vector<IVertexLayout::Attribute>& attributes() const override { return m_attrs; }

private:
    std::vector<IVertexLayout::Attribute> m_attrs;
    uint32_t m_stride;
};


class ObjVertexData : public IVertexData {
public:
    ObjVertexData(std::vector<ObjVertex> verts) : m_vertices(std::move(verts)) {};

    const IVertexLayout& layout() const override { return s_layout; };
    uint32_t vertexCount() const override { return static_cast<uint32_t>(m_vertices.size()); };
    const void* rawData() const override { return m_vertices.data(); };
    uint32_t stride() const override { return s_layout.stride(); };

    const std::vector<ObjVertex>& cpuData() const { return m_vertices; }

private:
    std::vector<ObjVertex> m_vertices;
    static ObjVertexLayout s_layout;
};