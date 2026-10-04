#include "GlTFModelDecoder.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../../external/tiny_gltf.h"

#include "../Vertex/ObjVertexData.h"
#include "../Vertex/GlTFVertexData.h"

namespace {

	glm::vec3 readVec3(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec3(0.0f);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 3;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec3(f[0], f[1], f[2]);
	}

	glm::vec2 readVec2(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec2(0.0f);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 2;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec2(f[0], f[1]);
	}

	glm::vec4 readVec4f(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return glm::vec4(0.0f);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t stride = view.byteStride ? view.byteStride : sizeof(float) * 4;
		size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
		const float* f = reinterpret_cast<const float*>(&buffer.data[byteOffset]);
		return glm::vec4(f[0], f[1], f[2], f[3]);
	}

	glm::uvec4 readUVec4(const tinygltf::Model& model, const tinygltf::Accessor& accessor, size_t index) {
		glm::uvec4 out(0);
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint16_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint16_t* v = reinterpret_cast<const uint16_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4((uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2], (uint32_t)v[3]);
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint8_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint8_t* v = reinterpret_cast<const uint8_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4((uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2], (uint32_t)v[3]);
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			size_t stride = view.byteStride ? view.byteStride : sizeof(uint32_t) * 4;
			size_t byteOffset = view.byteOffset + accessor.byteOffset + index * stride;
			const uint32_t* v = reinterpret_cast<const uint32_t*>(&buffer.data[byteOffset]);
			out = glm::uvec4(v[0], v[1], v[2], v[3]);
		}
		return out;
	}

	// tinygltf decodes every image (external file, data uri or glb buffer) into img.image,
	// hand those pixels over as RGBA8, fall back to the file path if decoding didn't happen
	DecodedTexture readTexture(const tinygltf::Model& model, int textureIndex, const std::filesystem::path& modelDir) {
		DecodedTexture out{};
		if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size())) return out;

		const tinygltf::Texture& tex = model.textures[textureIndex];
		if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size())) return out;

		const tinygltf::Image& img = model.images[tex.source];

		if (!img.image.empty() && img.bits == 8 && img.component >= 1 && img.component <= 4) {
			const size_t pixelCount = static_cast<size_t>(img.width) * img.height;
			out.width = static_cast<uint32_t>(img.width);
			out.height = static_cast<uint32_t>(img.height);

			if (img.component == 4) {
				out.pixels = img.image;
			}
			else {
				// most devices don't support RGB only on Vulkan, expand to RGBA
				out.pixels.resize(pixelCount * 4);
				for (size_t i = 0; i < pixelCount; ++i) {
					const unsigned char* src = &img.image[i * img.component];
					unsigned char* dst = &out.pixels[i * 4];
					dst[0] = src[0];
					dst[1] = img.component >= 2 ? src[1] : src[0];
					dst[2] = img.component >= 3 ? src[2] : src[0];
					dst[3] = img.component == 2 ? src[1] : 255;
				}
			}
			return out;
		}

		if (!img.uri.empty() && img.uri.rfind("data:", 0) != 0) {
			std::filesystem::path imgPath(img.uri);
			if (imgPath.is_relative()) imgPath = modelDir / imgPath;
			out.path = imgPath.string();
		}
		else {
			std::cerr << "GLTF warning: unsupported image format for texture " << textureIndex << std::endl;
		}

		return out;
	}

	std::vector<uint32_t> readIndices(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
		std::vector<uint32_t> out;
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[view.buffer];
		size_t count = accessor.count;
		out.reserve(count);
		size_t byteOffsetBase = view.byteOffset + accessor.byteOffset;
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			for (size_t i = 0; i < count; ++i) {
				const uint16_t* v = reinterpret_cast<const uint16_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint16_t)]);
				out.push_back(static_cast<uint32_t>(*v));
			}
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			for (size_t i = 0; i < count; ++i) {
				const uint32_t* v = reinterpret_cast<const uint32_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint32_t)]);
				out.push_back(*v);
			}
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
			for (size_t i = 0; i < count; ++i) {
				const uint8_t* v = reinterpret_cast<const uint8_t*>(&buffer.data[byteOffsetBase + i * sizeof(uint8_t)]);
				out.push_back(static_cast<uint32_t>(*v));
			}
		}
		return out;
	}
}

bool GlTFModelDecoder::canDecode(const std::filesystem::path& path) const {
	auto ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
	return (ext == "gltf" || ext == "glb");
}

DecodedModel GlTFModelDecoder::decode(const std::filesystem::path& path) const {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = false;
	auto ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
	if (ext == "glb") {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	} else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	}

	if (!warn.empty()) std::cerr << "GLTF warning: " << warn << std::endl;
	if (!err.empty()) std::cerr << "GLTF error: " << err << std::endl;
	if (!ret) throw std::runtime_error("Failed to load glTF file: " + path.string());

	DecodedModel out{};
	std::vector<GltfVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Primitive> primitives;
	std::vector<DecodedMaterial> materials;

	// extract materials
	for (const auto& mat : model.materials) {
		DecodedMaterial dm;
		dm.name = mat.name;
		dm.albedoTexture = readTexture(model, mat.pbrMetallicRoughness.baseColorTexture.index, path.parent_path());
		dm.normalTexture = readTexture(model, mat.normalTexture.index, path.parent_path());
		dm.metallicRoughnessTexture = readTexture(model, mat.pbrMetallicRoughness.metallicRoughnessTexture.index, path.parent_path());
		dm.occlusionTexture = readTexture(model, mat.occlusionTexture.index, path.parent_path());
		dm.emissiveTexture = readTexture(model, mat.emissiveTexture.index, path.parent_path());
		dm.emissiveFactor = glm::vec3(
			static_cast<float>(mat.emissiveFactor[0]),
			static_cast<float>(mat.emissiveFactor[1]),
			static_cast<float>(mat.emissiveFactor[2])
		);
		dm.alphaMask = mat.alphaMode == "MASK";
		dm.alphaCutoff = static_cast<float>(mat.alphaCutoff);
		dm.metallic = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
		dm.roughness = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);
		dm.baseColorFactor = glm::vec4(
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]),
			static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[3])
		);
		materials.push_back(std::move(dm));
	}

	// iterate meshes/primitives
	for (const auto& mesh : model.meshes) {
		for (const auto& prim : mesh.primitives) {
			size_t primFirstIndex = indices.size();
			size_t primStartVertex = vertices.size();

			// find accessors
			const auto itPos = prim.attributes.find("POSITION");
			if (itPos == prim.attributes.end()) continue; // invalid primitive
			const tinygltf::Accessor& posAcc = model.accessors[itPos->second];

			const tinygltf::Accessor* normalAcc = nullptr;
			const tinygltf::Accessor* tex0Acc = nullptr;
			const tinygltf::Accessor* tex1Acc = nullptr;
			const tinygltf::Accessor* jointsAcc = nullptr;
			const tinygltf::Accessor* weightsAcc = nullptr;
			const tinygltf::Accessor* colorAcc = nullptr;

			auto it = prim.attributes.find("NORMAL"); if (it != prim.attributes.end()) normalAcc = &model.accessors[it->second];
			it = prim.attributes.find("TEXCOORD_0"); if (it != prim.attributes.end()) tex0Acc = &model.accessors[it->second];
			it = prim.attributes.find("TEXCOORD_1"); if (it != prim.attributes.end()) tex1Acc = &model.accessors[it->second];
			it = prim.attributes.find("JOINTS_0"); if (it != prim.attributes.end()) jointsAcc = &model.accessors[it->second];
			it = prim.attributes.find("WEIGHTS_0"); if (it != prim.attributes.end()) weightsAcc = &model.accessors[it->second];
			it = prim.attributes.find("COLOR_0"); if (it != prim.attributes.end()) colorAcc = &model.accessors[it->second];

			std::vector<uint32_t> primIndices;
			if (prim.indices >= 0) {
				const tinygltf::Accessor& idxAcc = model.accessors[prim.indices];
				primIndices = readIndices(model, idxAcc);
			}

			if (!primIndices.empty()) {
				// create one vertex per index (no dedup)
				for (size_t k = 0; k < primIndices.size(); ++k) {
					uint32_t idx = primIndices[k];
					GltfVertex v{};
					v.position = readVec3(model, posAcc, idx);
					if (normalAcc) v.normal = readVec3(model, *normalAcc, idx);
					if (tex0Acc) v.uv0 = readVec2(model, *tex0Acc, idx);
					if (colorAcc) {
						glm::vec4 c = readVec4f(model, *colorAcc, idx);
						v.color = glm::vec3(c.r, c.g, c.b);
					}
					vertices.push_back(v);
					indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
				}

				Primitive p;
				p.firstIndex = static_cast<uint32_t>(primFirstIndex);
				p.indexCount = static_cast<uint32_t>(primIndices.size());
				p.materialIndex = prim.material >= 0 ? static_cast<uint32_t>(prim.material) : 0;
				primitives.push_back(p);
			}
			else {
				// no indices -> use accessor count
				size_t count = posAcc.count;
				for (size_t k = 0; k < count; ++k) {
					GltfVertex v{};
					v.position = readVec3(model, posAcc, k);
					if (normalAcc) v.normal = readVec3(model, *normalAcc, k);
					if (tex0Acc) v.uv0 = readVec2(model, *tex0Acc, k);
					if (colorAcc) {
						glm::vec4 c = readVec4f(model, *colorAcc, k);
						v.color = glm::vec3(c.r, c.g, c.b);
					}
					vertices.push_back(v);
					indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
				}

				Primitive p;
				p.firstIndex = static_cast<uint32_t>(primFirstIndex);
				p.indexCount = static_cast<uint32_t>(count);
				p.materialIndex = prim.material >= 0 ? static_cast<uint32_t>(prim.material) : 0;
				primitives.push_back(p);
			}
		}
	}

	// compute aabb
	glm::vec3 minV(std::numeric_limits<float>::infinity());
	glm::vec3 maxV(-std::numeric_limits<float>::infinity());
	for (const auto& v : vertices) {
		minV = glm::min(minV, v.position);
		maxV = glm::max(maxV, v.position);
	}

	out.vertices = std::make_unique<GltfVertexData>(std::move(vertices));
	out.indices = std::move(indices);
	out.primitives = std::move(primitives);
	out.materials = std::move(materials);
	out.aabb = BoundingBox(minV, maxV);
	out.aabb.valid = true;
	out.name = path.filename().string();

	return out;
}
