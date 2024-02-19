#pragma once

#include "Camera.h"
#include "GameObject.h"

#include <vulkan/vulkan.h>

#define MAX_LIGHT 10

struct PointLight {
	glm::vec4 position{};
	glm::vec4 color{};
};

struct GlobalUbo {
	glm::mat4 projection{ 1.0f };
	glm::mat4 view{ 1.0f };
	glm::mat4 inverseView{ 1.f };
	glm::vec4 ambientLightColor{ 1.f, 1.f,  1.f, .2f };
	PointLight pointLights[MAX_LIGHT];
	int numLights;
};


struct FrameInfo {
	int frameIndex;
	float frameTime;
	VkCommandBuffer commandBuffer;
	Camera& camera;
	VkDescriptorSet globalDescriptorSet;
	GameObject::Map& gameObjects;
};


