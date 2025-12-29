#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "BoundingBox.h"
#include "../base/Buffer.h"
#include "ModelAnimation.h"
#include "../base/Device.h"

#define MAX_NUM_JOINTS 64u

class ModelUploader;


struct Primitive {
	BoundingBox aabb;

	uint32_t firstIndex;
	uint32_t indexCount;

	uint32_t materialIndex = 0;
};


struct NodeTransform {
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation;
};

struct UniformBlock {
	glm::mat4 matrix;
	glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
	uint32_t jointcount{ 0 };
};




class Node
{
public:
	~Node();

	std::string name;

	glm::mat4 localMatrix();
	glm::mat4 getMatrix();

	void update_cpu();
	void update_gpu();

	std::vector<Primitive> primitives;

	Node* parent;
	std::vector<Node*> children;

	int parentIndex = -1;
	std::vector<int> childrenIndices;

	glm::mat4 matrix{};
	NodeTransform transform;

	size_t index;
	
	bool hasSkin = false;
	Skin* skin = nullptr;
	int32_t skinIndex = -1;

	BoundingBox aabb;

	bool useCachedMatrix{ true };
	glm::mat4 cachedLocalMatrix{ glm::mat4(1.0f) };
	glm::mat4 cachedMatrix{ glm::mat4(1.0f) };

	bool bufferCreated = false;
	UniformBlock uniformBlock;
	std::unique_ptr<Buffer> uniformBuffer;
	std::vector<VkDescriptorSet> descriptorSet;

	friend ModelUploader;


//private:
	void createBuffer(bool hasSkin, Device& device);
	void updateUniformBuffer();
	void updateStaticUniform(glm::mat4& worldMatrix);
	void updateSkinnedUniforms(const glm::mat4& worldMatrix);
};