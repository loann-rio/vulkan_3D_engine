#pragma once

#include "model.h"

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

	static GameObject createGameObject() {
		static id_t currentId = 0;
		return GameObject{ currentId++ };
	}

	static GameObject makePointLight(float intencity, float radius, glm::vec3 color);

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	id_t getId() { return id; }

	TransformComponent transform{};
	glm::vec3 color{};

	std::shared_ptr<Model> model{};
	std::unique_ptr<PointLightComponent> pointLight = nullptr;

private:
	GameObject(id_t obId) : id{obId} {}
	id_t id;
};