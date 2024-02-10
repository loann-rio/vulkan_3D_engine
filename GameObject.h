#pragma once

#include "model.h"

#include <memory>

struct Transform2dComponent {
	glm::vec2 translation{};
	glm::vec2 scale{ 1.f, 1.f };
	float rotation;

	glm::mat2 mat2() {
		const float s = glm::sin(rotation);
		const float c = glm::cos(rotation);
		glm::mat2 rotMatrix{ {c, s}, {-s, c} };
		glm::mat2 scaleMat{ {scale.x, .0f}, {.0f, scale.y} };

		return rotMatrix * scaleMat;
	}
};

class GameObject
{
public:

	using id_t = unsigned int;

	static GameObject createGameObject() {
		static id_t currentId = 0;
		return GameObject{ currentId++ };
	}

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	std::shared_ptr<Model> model{};
	glm::vec3 color{};

	id_t getId() { return id; }

	Transform2dComponent transform2d{};

private:
	GameObject(id_t obId) : id{obId} {}
	id_t id;

};

