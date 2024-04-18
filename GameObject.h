#pragma once

#include "model.h"
#include "Swap_chain.h"
#include "descriptors.h"
#include "Device.h"
#include "Texture.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <iostream>
#include <unordered_map>

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 scale{ 1.f, 1.f , 1.f};
	glm::vec3 rotation{};

	glm::mat4 mat4();
	glm::mat3 normalMatrix();
};

struct PointLightComponent {
	float LightIntencity = 1.0f;

};

class GameObject
{
public:

	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, GameObject>;

	static GameObject createGameObject(Device& device) {
		static id_t currentId = 0;
		return GameObject{ currentId++, device};
	}

	static GameObject makePointLight(Device& device, float intencity, float radius, glm::vec3 color);

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	id_t getId() { return id; }

	TransformComponent transform{};
	glm::vec3 color{};

	std::shared_ptr<Model> model{};
	std::unique_ptr<PointLightComponent> pointLight = nullptr;

	std::vector<VkDescriptorSet> descriptorSet{ Swap_chain::MAX_FRAMES_IN_FLIGHT };

	void createDescriptorSet(DescriptorPool& pool, Device& device);


private:

	Device& device;

	GameObject(id_t obId, Device& device) : id{obId} , device{device} {}

	id_t id;
};