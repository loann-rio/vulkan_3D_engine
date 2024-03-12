#pragma once

#include "model.h"
#include "Swap_chain.h"
#include "descriptors.h"
#include "Device.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
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
		return GameObject{ currentId++, device };
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

	std::unique_ptr<DescriptorSetLayout> setLayout = nullptr;


private:

	Device& device;

	GameObject(id_t obId, Device& device) : id{obId} , device{device}
	{
		setLayout = DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

	}
	id_t id;
};