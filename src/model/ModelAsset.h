#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "../base/Buffer.h"

#include "Decoder/IModelDecoder.h"

#include "ModelNode.h"

class ModelBuilder;

struct ModelLOD {
	ModelLOD(const ModelLOD&) = delete;
	ModelLOD& operator=(const ModelLOD&) = delete;

	ModelLOD(ModelLOD&&) noexcept = default;
	ModelLOD& operator=(ModelLOD&&) noexcept = default;

	bool updateAnimation(uint32_t index, float timeInSeconds, float speed = 1.0f);


	// Vertex Buffer
	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	// Index Buffer
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;

	// animation
	std::vector<Animation> animations;
	std::vector<std::unique_ptr<Skin>> skins;

	uint32_t vertexStride = 0; // size of single vertex

	std::vector<std::unique_ptr<Node>> LinearNodes{};
	std::vector<Node*> nodes{}; // contains pointer to the root nodes of the model
	std::vector<Material> materials;

	bool hasDescriptor = false;

	BoundingBox aabb;

	float switchDistance = std::numeric_limits<float>::infinity();
};

class ModelAsset {
public:
	ModelAsset() = default;
	~ModelAsset() = default;

	ModelLOD& baseLod() { return lods[0]; }

//private:
	bool hasShadow = false;

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

