#include "GlTFModelDecoder.h"

#include <iostream>
#include "../Vertex/GltfVertexData.h"

namespace {
	void generateLocalNodeMatrix(const tinygltf::Node node, Node* newNode) {
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

	void resolveFloatAttrib(const tinygltf::Primitive& primitive, tinygltf::Model gltfModel, const char* name, const float*& buffer, int& stride, int components) {
		auto it = primitive.attributes.find(name);
		if (it == primitive.attributes.end()) return;

		const tinygltf::Accessor& acc = gltfModel.accessors[it->second];
		const tinygltf::BufferView& view = gltfModel.bufferViews[acc.bufferView];
		buffer = reinterpret_cast<const float*>(&gltfModel.buffers[view.buffer].data[acc.byteOffset + view.byteOffset]);
		stride = acc.ByteStride(view) ? acc.ByteStride(view) / sizeof(float) : components;
	}

	std::vector<Primitive> loadMesh(const tinygltf::Mesh mesh,
		tinygltf::Model gltfModel,
		std::vector<GltfVertex>& localVertices,
		std::vector<uint32_t>& localIndices) {

		std::vector<Primitive> primitives;

		primitives.reserve(mesh.primitives.size());

		bool hasSkin = false;

		for (size_t j = 0; j < mesh.primitives.size(); j++)
		{
			const tinygltf::Primitive& primitive = mesh.primitives[j];

			auto posIt = primitive.attributes.find("POSITION");
			if (posIt == primitive.attributes.end()) {
				throw std::runtime_error("glTF primitive has no POSITION attribute");
			}

			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;

			glm::vec3 posMin{};
			glm::vec3 posMax{};


			bool hasIndices = primitive.indices > -1;

			const uint32_t vertexStart = static_cast<uint32_t>(localVertices.size());
			const uint32_t indexStart = static_cast<uint32_t>(localIndices.size());

			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTexCoordSet0 = nullptr;
				const float* bufferTexCoordSet1 = nullptr;
				const float* bufferColorSet0 = nullptr;
				const void* bufferJoints = nullptr;
				const float* bufferWeights = nullptr;

				int posByteStride;
				int normByteStride;
				int uv0ByteStride;
				int uv1ByteStride;
				int color0ByteStride;
				int jointByteStride;
				int weightByteStride;

				int jointComponentType;

				// Position attribute is required
				assert(primitive.attributes.find("POSITION") != primitive.attributes.end() && "Position attribute is required");

				const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];



				bufferPos = reinterpret_cast<const float*>(&(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));

				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

				vertexCount = static_cast<uint32_t>(posAccessor.count);
				posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);


				if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
					resolveFloatAttrib(primitive, gltfModel, "NORMAL", bufferNormals, normByteStride, TINYGLTF_TYPE_VEC3);

				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
					resolveFloatAttrib(primitive, gltfModel, "TEXCOORD_0", bufferTexCoordSet0, uv0ByteStride, TINYGLTF_TYPE_VEC2);

				if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
					resolveFloatAttrib(primitive, gltfModel, "TEXCOORD_1", bufferTexCoordSet1, uv1ByteStride, TINYGLTF_TYPE_VEC2);

				if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
					resolveFloatAttrib(primitive, gltfModel, "COLOR_0", bufferColorSet0, color0ByteStride, TINYGLTF_TYPE_VEC3);

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
					resolveFloatAttrib(primitive, gltfModel, "WEIGHTS_0", bufferWeights, weightByteStride, TINYGLTF_TYPE_VEC4);

				// Skinning
				// Joints
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& jointAccessor = gltfModel.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& jointView = gltfModel.bufferViews[jointAccessor.bufferView];
					bufferJoints = &(gltfModel.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
					jointComponentType = jointAccessor.componentType;
					jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				hasSkin = hasSkin || (bufferJoints && bufferWeights);

				//// build vertices ////

				for (size_t v = 0; v < posAccessor.count; v++)
				{
					GltfVertex vert;

					vert.position = glm::make_vec3(&bufferPos[v * posByteStride]);

					vert.normal = bufferNormals
						? glm::normalize(glm::make_vec3(&bufferNormals[v * normByteStride]))
						: glm::vec3(0.0f);

					vert.uv0 = bufferTexCoordSet0
						? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride])
						: glm::vec2(0.0f);

					vert.uv1 = bufferTexCoordSet1
						? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride])
						: glm::vec2(0.0f);

					vert.color = bufferColorSet0
						? glm::make_vec3(&bufferColorSet0[v * color0ByteStride])
						: glm::vec3(1.0f);

					if (hasSkin)
					{
						switch (jointComponentType) {
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
							vert.joint0 = glm::uvec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
							vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						default:
							// Not supported by spec
							vert.joint0 = glm::uvec4(0);
							std::cerr << "Joint component type " << jointComponentType << " not supported!" << std::endl;
							break;
						}
					}
					else {
						vert.joint0 = glm::vec4(0.0f);
					}

					vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);

					// Fix for all zero weights
					if (glm::length(vert.weight0) == 0.0f) {
						vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					}

					localVertices.push_back(vert);
				}
			}

			// Indices
			if (hasIndices)
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				for (size_t i = 0; i < indexCount; ++i) {
					uint32_t index = 0;

					switch (accessor.componentType) {
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						index = static_cast<const uint32_t*>(dataPtr)[i];
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						index = static_cast<const uint16_t*>(dataPtr)[i];
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						index = static_cast<const uint8_t*>(dataPtr)[i];
						break;
					default:
						throw std::runtime_error("Unsupported index component type");
					}

					localIndices.push_back(vertexStart + index);
				}
			}


			Primitive newPrimitive;
			newPrimitive.firstIndex = indexStart;
			newPrimitive.indexCount = indexCount;
			newPrimitive.materialIndex = primitive.material > -1 ? primitive.material : 0;

			newPrimitive.aabb = BoundingBox{ posMin, posMax };

			primitives.push_back(newPrimitive);
		}


		return primitives;
	}

}

void GlTFModelDecoder::loadNode(Node* parentNode, const tinygltf::Node node, size_t nodeIndex, tinygltf::Model gltfModel, std::vector<GltfVertex>& localVertices, std::vector<uint32_t>& localIndices, std::vector<std::unique_ptr<Node>>& nodes)
{
	std::unique_ptr<Node> newNode{};

	newNode->index = nodeIndex;
	newNode->parent = parentNode;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	generateLocalNodeMatrix(node, newNode.get());

	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			loadNode(newNode.get(), gltfModel.nodes[node.children[i]], node.children[i], gltfModel, localVertices, localIndices, nodes);
		}
	}

	std::vector<Primitive> primitives;
	if (node.mesh > -1)
		primitives = loadMesh(gltfModel.meshes[node.mesh], gltfModel, localVertices, localIndices);

	for (auto& p : primitives) {
		if (p.aabb.valid && !newNode->aabb.valid) {
			newNode->aabb = p.aabb;
			newNode->aabb.valid = true;
		}
		newNode->aabb.min = glm::min(newNode->aabb.min, p.aabb.min);
		newNode->aabb.max = glm::max(newNode->aabb.max, p.aabb.max);
	}


	newNode->primitives = std::move(primitives);

	if (parentNode) parentNode->children.push_back(newNode.get());

	nodes.push_back(std::move(newNode));
}