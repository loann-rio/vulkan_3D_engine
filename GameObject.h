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

typedef enum ModelType {
	UNDEFINED_MODEL = 0,
	OBJ_MODEL = 1,
	GLTF_MODEL = 2,
	QUAD_MODEL = 3 
};

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
	ModelType modelType = UNDEFINED_MODEL;
	ModelVariant model;

	std::unique_ptr<PointLightComponent> pointLight = nullptr;

	template <typename T>
	void setModel(std::shared_ptr<T> newModel);
	void setModelType(ModelType type) { modelType = type; }

	void createDescriptorSet(DescriptorPool& pool) const;

	std::vector<VkDescriptorSet> getDescriptorSets() const;
	uint16_t getDescriptorSetIndex() const;

	void bindModel(VkCommandBuffer& commandBuffer) const;
	void drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& GlTFPipelineLayout) const;

private:
	Device& device;
	GameObject(id_t obId, Device& device) : id{obId} , device{device} {}
	id_t id;
};

template<typename T>
inline void GameObject::setModel(std::shared_ptr<T> newModel)
{
	model = std::move(newModel);
	modelType = static_cast<ModelType>(T::getModelType());
	hasModel = true;
}
