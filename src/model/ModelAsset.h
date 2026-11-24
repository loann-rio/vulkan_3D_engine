#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>


#include "../base/Buffer.h"
#include "../assetManager/TextureManager.h"

#include "Decoder/IModelDecoder.h"
#include "Animation/Skeleton.h"
#include "BoundingBox.h"

class ModelBuilder;

constexpr uint32_t MAX_MODEL_TEXTURES = 64u;

struct alignas(16) ShaderMaterialData {
	glm::vec4 baseColorFactor{ 1.f };
	glm::vec4 emissiveFactor{ 0.f };
	glm::vec4 diffuseFactor{ 1.f };
	glm::vec4 specularFactor{ 0.f };
	float workflow = 0.f;

	int baseColorTextureSet = -1;
	int physicalDescriptorTextureSet = -1;
	int normalTextureSet = -1;
	int occlusionTextureSet = -1;
	int emissiveTextureSet = -1;

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

struct Mesh {
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	BoundingBox aabb;

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
};

class ModelAsset {
public:
	ModelAsset() = default;
	~ModelAsset() = default;

	BoundingBox getAABB(glm::mat4 transform) { return mesh.aabb.getAABB(transform); }
	bool useAABB() const { return mesh.aabb.valid; }

	bool hasShadow = true;

	Mesh mesh;
	Skeleton skeleton;

	bool isAnimated() const { return !skeleton.clips.empty(); }

private:

	friend ModelBuilder;
};

