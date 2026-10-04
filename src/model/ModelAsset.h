#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "../base/Buffer.h"

#include "Decoder/IModelDecoder.h"

class ModelBuilder;

// max number of textures bound in a model texture array (set 2, binding 0 of the gltf shader)
constexpr uint32_t MAX_MODEL_TEXTURES = 64u;

// GPU side material, same layout as ShaderMaterial in the gltf shaders (std430)
struct alignas(16) ShaderMaterialData {
	glm::vec4 baseColorFactor{ 1.f };
	glm::vec4 emissiveFactor{ 0.f };
	glm::vec4 diffuseFactor{ 1.f };
	glm::vec4 specularFactor{ 0.f };
	float workflow = 0.f;

	// -1 = texture not used, >= 0 index of the texture coordinate set
	int baseColorTextureSet = -1;
	int physicalDescriptorTextureSet = -1;
	int normalTextureSet = -1;
	int occlusionTextureSet = -1;
	int emissiveTextureSet = -1;

	// index in ModelLOD::textures
	int baseColorTextureIndex = -1;
	int metallicRoughnessTextureIndex = -1;
	int normalTextureIndex = -1;
	int occlusionTextureIndex = -1;
	int emissiveTextureIndex = -1;

	float metallicFactor = 0.f;
	float roughnessFactor = 1.f;
	float alphaMask = 0.f;
	float alphaMaskCutoff = 0.5f;
	float emissiveStrength = 1.f;
};

struct ModelLOD {
	ModelLOD(const ModelLOD&) = delete;
	ModelLOD& operator=(const ModelLOD&) = delete;

	ModelLOD(ModelLOD&&) noexcept = default;
	ModelLOD& operator=(ModelLOD&&) noexcept = default;


	// Vertex Buffer
	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	// Index Buffer
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;

	uint32_t vertexStride = 0; // size of single vertex

	std::vector<Primitive> primitives;
	std::vector<Material> materials;

	std::vector<TextureManager::TextureID> textures;
	std::unique_ptr<Buffer> materialBuffer;
	std::vector<VkDescriptorSet> descriptorSet;

	float switchDistance = std::numeric_limits<float>::infinity();
};

class ModelAsset {
public:
	ModelAsset() = default;
	~ModelAsset() = default;

//private:
	bool hasShadow = true;

	// LODs
	std::vector<ModelLOD> lods;

	// Axis Aligned Bounding Box
	BoundingBox aabb;

	size_t pickLODIndex(const glm::vec3& cameraPosition, const glm::vec3& worldPosition) const {
		if (lods.empty()) return 0;
		float d = glm::length(cameraPosition - worldPosition);
		for (size_t i = 0; i < lods.size(); ++i) {
			if (d < lods[i].switchDistance) return i;
		}
		return lods.size() - 1;
	}


	friend ModelBuilder;
};

