#pragma once

#include "../render/Camera.h"
#include "../objects/GameObject.h"

#include <vulkan/vulkan.h>

#define MAX_LIGHT 10
#define MAX_SPOTLIGHT 4

struct PointLight {
	glm::vec4 position{};
	glm::vec4 color{};
};

struct SpotLightUbo {
	SpotLight spotLight[MAX_SPOTLIGHT];
	int numLights;
};

struct GlobalUbo {
	glm::mat4 projection{ 1.0f };
	glm::mat4 view{ 1.0f };
	glm::mat4 inverseView{ 1.f };
	glm::vec4 ambientLightColor{ 1.f, 1.f,  1.f, .1f };
	glm::vec4 globalLightDir{ 1.f, -3.f, 0.5f, 0.f };
	glm::vec3 lightPos{ 0.f, 0.f, 0.f };
	int numLights;
	PointLight pointLights[MAX_LIGHT];
};

struct FrameInfo {
	int frameIndex;
	float frameTime;
	int spotLightCount;
	glm::vec3 cameraPos;
	std::vector<GameObjectModel*> listGameObjects; 
};
