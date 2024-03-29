#pragma once

#include "Camera.h"
#include "GameObject.h"

#include <vulkan/vulkan.h>

#define MAX_LIGHT 10

struct PointLight {
	glm::vec4 position{};
	glm::vec4 color{};
	//glm::vec4 direction{};
	//float angle;
};

struct GlobalUbo {
	glm::mat4 projection{ 1.0f };
	glm::mat4 view{ 1.0f };
	glm::mat4 inverseView{ 1.f };
	glm::vec4 ambientLightColor{ .97f, .97f,  1.f, .5f };
	PointLight pointLights[MAX_LIGHT];
	glm::vec4 globalLightDir{ 1.f, -3.f, 0.5f, 1.f };
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
