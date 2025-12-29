#include "GlTFModelDecoder.h"

#include <iostream>
#include "../Vertex/GltfVertexData.h"
#include <glm/gtc/type_ptr.hpp>

#include "../ModelNode.h"

namespace {
	struct AttribView {
		const uint8_t* data = nullptr;
		uint32_t stride = 0;
		int componentType = 0;
		bool normalized = false;
	};

	static AttribView getAttrib(
		const tinygltf::Primitive& prim,
		const tinygltf::Model& model,
		const char* name)
	{
		auto it = prim.attributes.find(name);
		if (it == prim.attributes.end()) return {};

		const tinygltf::Accessor& acc = model.accessors[it->second];
		const tinygltf::BufferView& view = model.bufferViews[acc.bufferView];
		const tinygltf::Buffer& buf = model.buffers[view.buffer];

		AttribView a;
		a.data = buf.data.data() + view.byteOffset + acc.byteOffset;
		a.stride = acc.ByteStride(view);
		if (a.stride == 0)
			a.stride = tinygltf::GetComponentSizeInBytes(acc.componentType) *
			tinygltf::GetNumComponentsInType(acc.type);

		a.componentType = acc.componentType;
		a.normalized = acc.normalized;
		return a;
	}

	static float readComponent(const uint8_t* p, int type, bool norm)
	{
		switch (type) {
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return *reinterpret_cast<const float*>(p);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return norm ? (*p / 255.0f) : float(*p);
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			return norm ? glm::max(float(*reinterpret_cast<const int8_t*>(p)) / 127.0f, -1.0f)
				: float(*reinterpret_cast<const int8_t*>(p));
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return norm ? (*reinterpret_cast<const uint16_t*>(p) / 65535.0f)
				: float(*reinterpret_cast<const uint16_t*>(p));
		case TINYGLTF_COMPONENT_TYPE_SHORT:
			return norm ? glm::max(float(*reinterpret_cast<const int16_t*>(p)) / 32767.0f, -1.0f)
				: float(*reinterpret_cast<const int16_t*>(p));
		default:
			return 0.0f;
		}
	}


	void generateLocalNodeMatrix(const tinygltf::Node& node, Node* newNode) {
		// Generate local node matrix
		glm::vec3 translation = glm::vec3(0.0f);
		if (node.translation.size() == 3) {
			translation = glm::make_vec3(node.translation.data());
			newNode->transform.translation = translation;
		}

		glm::mat4 rotation = glm::mat4(1.0f);
		if (node.rotation.size() == 4) {
			glm::quat q = glm::make_quat(node.rotation.data());
			newNode->transform.rotation = glm::mat4(q);
		}

		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
			newNode->transform.scale = scale;
		}

		if (node.matrix.size() == 16) {
			newNode->matrix = glm::make_mat4x4(node.matrix.data());
		};
	}

	void resolveFloatAttrib(const tinygltf::Primitive& primitive, tinygltf::Model& gltfModel, const char* name, const float*& buffer, int& stride, int components) {
		auto it = primitive.attributes.find(name);
		if (it == primitive.attributes.end()) return;

		const tinygltf::Accessor& acc = gltfModel.accessors[it->second];
		const tinygltf::BufferView& view = gltfModel.bufferViews[acc.bufferView];
		buffer = reinterpret_cast<const float*>(&gltfModel.buffers[view.buffer].data[acc.byteOffset + view.byteOffset]);
		stride = acc.ByteStride(view) ? acc.ByteStride(view) / sizeof(float) : components;
	}

    static uint32_t componentSize(int componentType)
    {
        return tinygltf::GetComponentSizeInBytes(componentType);
    }


    std::vector<Primitive> loadMesh(
        const tinygltf::Mesh& mesh,
        const tinygltf::Model& model,
        std::vector<GltfVertex>& vertices,
        std::vector<uint32_t>& indices)
    {
        std::vector<Primitive> result;

        for (const auto& prim : mesh.primitives) {
            if (prim.attributes.count("POSITION") == 0)
                throw std::runtime_error("Primitive missing POSITION");

            const uint32_t vertexStart = uint32_t(vertices.size());
            const uint32_t indexStart = uint32_t(indices.size());

            AttribView posA = getAttrib(prim, model, "POSITION");
            AttribView nrmA = getAttrib(prim, model, "NORMAL");
            AttribView uv0A = getAttrib(prim, model, "TEXCOORD_0");
            AttribView uv1A = getAttrib(prim, model, "TEXCOORD_1");
            AttribView colA = getAttrib(prim, model, "COLOR_0");
            AttribView wgtA = getAttrib(prim, model, "WEIGHTS_0");

            AttribView jntA{};
            if (prim.attributes.count("JOINTS_0")) {
                const auto& acc = model.accessors[prim.attributes.at("JOINTS_0")];
                const auto& view = model.bufferViews[acc.bufferView];
                const auto& buf = model.buffers[view.buffer];

                jntA.data = buf.data.data() + view.byteOffset + acc.byteOffset;
                jntA.stride = acc.ByteStride(view);
                if (!jntA.stride)
                    jntA.stride = tinygltf::GetComponentSizeInBytes(acc.componentType) * 4;
                jntA.componentType = acc.componentType;
            }

            const size_t vCount =
                model.accessors[prim.attributes.at("POSITION")].count;

            for (size_t i = 0; i < vCount; ++i) {
                GltfVertex v{};


                {
                    assert(posA.stride >= componentSize(posA.componentType) * 3);

                    const uint32_t csz = componentSize(posA.componentType);
                    const uint8_t* p = posA.data + i * posA.stride;
                    v.position = { 
                        readComponent(p + 0 * csz, posA.componentType, false),
                        readComponent(p + 1 * csz, posA.componentType, false),
                        readComponent(p + 2 * csz, posA.componentType, false)
                    };
                }

                if (nrmA.data) {
                    const uint32_t csz = componentSize(nrmA.componentType);
                    const uint8_t* n = nrmA.data + i * nrmA.stride;
                    v.normal = glm::normalize(glm::vec3(
                        readComponent(n + 0 * csz, nrmA.componentType, nrmA.normalized),
                        readComponent(n + 1 * csz, nrmA.componentType, nrmA.normalized),
                        readComponent(n + 2 * csz, nrmA.componentType, nrmA.normalized)
                    ));
                }

                if (uv0A.data) {
                    const uint32_t csz = componentSize(uv0A.componentType);
                    const uint8_t* u = uv0A.data + i * uv0A.stride;
                    v.uv0 = {
                        readComponent(u + 0 * csz, uv0A.componentType, uv0A.normalized),
                        readComponent(u + 1 * csz, uv0A.componentType, uv0A.normalized)
                    };
                }

                /*if (uv1A.data) {
                    const uint32_t csz = componentSize(uv1A.componentType);
                    const uint8_t* u = uv1A.data + i * uv1A.stride;
                    v.uv1 = {
                        readComponent(u + 0 * csz, uv1A.componentType, uv1A.normalized),
                        readComponent(u + 1 * csz, uv1A.componentType, uv1A.normalized)
                    };
                }*/

                if (colA.data) {
                    const uint32_t csz = componentSize(colA.componentType);
                    const uint8_t* c = colA.data + i * colA.stride;
                    v.color = {
                        readComponent(c + 0 * csz, colA.componentType, colA.normalized),
                        readComponent(c + 1 * csz, colA.componentType, colA.normalized),
                        readComponent(c + 2 * csz, colA.componentType, colA.normalized)
                    };
                }

                /*if (jntA.data && wgtA.data) {
                    const uint8_t* j = jntA.data + i * jntA.stride;
                    const uint8_t* w = wgtA.data + i * wgtA.stride;

                    for (int k = 0; k < 4; ++k) {
                        v.joint0[k] = (jntA.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                            ? reinterpret_cast<const uint16_t*>(j)[k]
                            : reinterpret_cast<const uint8_t*>(j)[k];

                        const uint32_t csz = componentSize(wgtA.componentType);

                        v.weight0[k] = readComponent(
                            w + k * csz, wgtA.componentType, true);
                    }
                }*/

                vertices.push_back(v);
            }

            if (prim.indices >= 0) {
                const auto& acc = model.accessors[prim.indices];
                const auto& view = model.bufferViews[acc.bufferView];
                const auto& buf = model.buffers[view.buffer];
                const uint8_t* base = buf.data.data() + view.byteOffset + acc.byteOffset;

                for (size_t i = 0; i < acc.count; ++i) {
                    uint32_t idx = 0;
                    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        idx = reinterpret_cast<const uint16_t*>(base)[i];
                    else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                        idx = reinterpret_cast<const uint8_t*>(base)[i];
                    else
                        idx = reinterpret_cast<const uint32_t*>(base)[i];

                    indices.push_back(vertexStart + idx);
                }
            }

            Primitive p{};
            p.firstIndex = indexStart;
            p.indexCount = uint32_t(indices.size() - indexStart);
            p.materialIndex = prim.material >= 0 ? prim.material : 0;
            result.push_back(p);
        }

        return result;
    }


    BoundingBox calculateAABBFromPrimitives(std::vector<Primitive>& primitives) {
        BoundingBox aabb{};

        for (auto& p : primitives) {
            if (!p.aabb.valid) continue;

            if (!aabb.valid) {
                aabb = p.aabb;
                aabb.valid = true;
            }
            else
            {
                aabb.min = glm::min(aabb.min, p.aabb.min);
                aabb.max = glm::max(aabb.max, p.aabb.max);
            }
        }

        return aabb;
    }


}

void GlTFModelDecoder::loadNode(
    int parentNode, 
    const tinygltf::Node& node, 
    size_t nodeIndex, 
    tinygltf::Model& gltfModel, 
    std::vector<GltfVertex>& localVertices, 
    std::vector<uint32_t>& localIndices, 
    std::vector<std::unique_ptr<Node>>& nodes, 
    std::vector<size_t>& nodesGlTFIndex,
    std::vector<size_t>& rootNodes)
{
	std::unique_ptr<Node> newNode = std::make_unique<Node>();

	newNode->parentIndex = parentNode;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	generateLocalNodeMatrix(node, newNode.get());

	std::vector<Primitive> primitives;
	if (node.mesh > -1)
		primitives = loadMesh(gltfModel.meshes[node.mesh], gltfModel, localVertices, localIndices);

    newNode->primitives = std::move(primitives);
    newNode->aabb = calculateAABBFromPrimitives(newNode->primitives);

    const size_t node_index = nodes.size();

    nodesGlTFIndex.push_back(nodeIndex);
    nodes.push_back(std::move(newNode));

    if (node.children.size() > 0) 
        for (size_t i = 0; i < node.children.size(); i++) 
            loadNode(node_index, 
                gltfModel.nodes[node.children[i]], node.children[i], 
                gltfModel, 
                localVertices, localIndices,
                nodes, 
                nodesGlTFIndex, 
                rootNodes);


	if (parentNode != -1) 
        nodes[parentNode]->childrenIndices.push_back(node_index);
	else 
        rootNodes.push_back(node_index);	
}