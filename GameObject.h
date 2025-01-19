#pragma once

#include "Model.h"
#include "Swap_chain.h"
#include "descriptors.h"
#include "Device.h"
#include "GlTFModel.h"
#include "Texture.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <iostream>
#include <type_traits>
#include <variant>
#include <unordered_map>

using ModelVariant = std::variant<std::shared_ptr<Model>, 
					std::shared_ptr<GlTFModel::ModelGltf>>;

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

	bool hasModel = false;
	std::string modelType;

	ModelVariant model;

	std::unique_ptr<PointLightComponent> pointLight = nullptr;

	template <typename T>
	void setModel(std::shared_ptr<T> newModel) {
		model = std::move(newModel);
		modelType = T::getType();
		hasModel = true;
	}

	void createDescriptorSet(DescriptorPool& pool) const {
		std::visit([&pool, &device = this->device](const auto& modelInstance) {
			if (modelInstance) {
				modelInstance->createDescriptorSet(pool, device);
			}
		}, model);
	}

	std::vector<VkDescriptorSet> getDescriptorSets() const {
		return std::visit([](const auto& modelInstance) -> std::vector<VkDescriptorSet> {
			if (modelInstance) {
				return modelInstance->getDescriptorSets();
			}
			return {};
		}, model);
	}

	uint16_t getDescriptorSetIndex() const {
		return std::visit([](const auto& modelInstance) -> uint16_t {
			if (modelInstance) {
				return modelInstance->descriptorSetIndex;
			}
			return 1;
		}, model);
	}

	void bindModel(VkCommandBuffer& commandBuffer) const {
		std::visit([&](const auto& modelInstance) {
			if (modelInstance) {
				modelInstance->bind(commandBuffer);
			}
		}, model);
	}

	void drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& GlTFPipelineLayout) const {
		std::visit([&](const auto& modelInstance) {
			if (modelInstance) {
				modelInstance->draw(commandBuffer, GlTFPipelineLayout);
			}
		}, model);
	}


private:

	Device& device;

	GameObject(id_t obId, Device& device) : id{obId} , device{device} {}

	id_t id;
};
