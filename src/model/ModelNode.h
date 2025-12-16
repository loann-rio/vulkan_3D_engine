#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "BoundingBox.h"


class Skin;
class Mesh;
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

class Node
{
public:
	~Node();

	std::string name;

	glm::mat4 localMatrix();
	glm::mat4 getMatrix();

	void update();  // TODO

	std::vector<Primitive> primitives;

	Node* parent;
	std::vector<Node*> children;

	glm::mat4 matrix{};
	NodeTransform transform;

	size_t index;
	
	std::shared_ptr<Skin> skin;
	int32_t skinIndex = -1;

	BoundingBox aabb;

	bool useCachedMatrix{ true };
	glm::mat4 cachedLocalMatrix{ glm::mat4(1.0f) };
	glm::mat4 cachedMatrix{ glm::mat4(1.0f) };
	
	friend ModelUploader;
};