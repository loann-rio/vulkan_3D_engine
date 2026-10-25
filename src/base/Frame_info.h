#pragma once

#include "../render/Camera.h"
#include "../objects/GameObject.h"
#include "../objects/objectManager.h"

#include <vulkan/vulkan.h>

constexpr uint32_t MAX_LIGHT = 10;
constexpr uint32_t MAX_SPOTLIGHT = 4;
constexpr uint32_t MAX_GPU_OBJECTS = 4096;

struct alignas(16) GpuObjectData {
	glm::mat4 model{ 1.f };
};

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

struct TerrainUbo {
	float clif_slop{ 0.76f };
	float height_grass{ 2.3f };
	float slope_snow{ 1.f };
	float height_grass_with_slope{ 2.5f };
	float height_dirt_with_slope{ 2.5f };
	float height_snow{ 2.5f };
};

struct FrameInfo {
	int frameIndex;
	int imageIndex;
	float frameTime;
	int spotLightCount;
	glm::vec3 cameraPos;
	std::vector<GameObjectModel*> listGameObjects; 
	std::vector<ComputeObject*> computesObjects;
	std::vector<std::array<FrustumPlane, 6>> listFrustrumPlanes;
	std::array<FrustumPlane, 6> mainCameraFrustrumPlanes;
	float gpuFrameRate;

	std::vector<VkDescriptorSet> globalDescriptorSet;
	std::vector<VkDescriptorSet> shadowDescriptorSet;
	std::vector<VkDescriptorSet> terrainDescriptorSet;
	std::vector<VkDescriptorSet> postProDescriptorSet;
};
