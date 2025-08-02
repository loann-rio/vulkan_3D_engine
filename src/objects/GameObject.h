#pragma once

#include "../base/Swap_chain.h"
#include "../base/descriptors.h"
#include "../base/Device.h"
#include "../model/GlTFModel.h"
#include "../model/Model.h"
#include "../render/Camera.h"


#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <iostream>
#include <type_traits>
#include <variant>
#include <unordered_map>

using ModelVariant = std::variant<std::shared_ptr<Model>,
	std::shared_ptr<GlTFModel::ModelGltf>>;

enum ModelType {
	UNDEFINED_MODEL = 0,
	OBJ_MODEL = 1,
	GLTF_MODEL = 2,
	QUAD_MODEL = 3 
};

enum class GameObjectType { 
	UNKNOWN,
	CAMERA,
	MODEL,
	SPOT_LIGHT,
	POINT_LIGHT 
};

struct SpotLight { 
	glm::vec4 position{};
	glm::vec4 color{ 1.0f };  
	glm::vec4 orientation{}; 
	glm::mat4 lightMatrix{ 1.0f }; 
}; 

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 scale{ 1.f, 1.f , 1.f};
	glm::vec3 rotation{};
	glm::vec4 color{};

	glm::mat4 mat4();
	glm::mat3 normalMatrix();
};

class GameObject
{
public:

	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, std::unique_ptr<GameObject>>;

	id_t getId() { return id; } 

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	GameObject(GameObject&&) = default; 
	GameObject& operator=(GameObject&&) = default;

	GameObject(id_t id, Device& device) : id(id), device(device) {}
	virtual ~GameObject() = default;

	virtual void debugUI() {}
	virtual GameObjectType getType() const { return GameObjectType::UNKNOWN; } 

	TransformComponent transform{};

	void setName(std::string newName) { name = newName; }
	std::string getName() const { return name; }

	void setParent(GameObject* parent) { parentObject = parent; }
	void setChild(GameObject* child) { assert(this != child); child->setParent(this); } 

	glm::mat4 getTransformMat();
	glm::mat3 getNormalMat();
	
protected:

	//std::vector<id_t> children{};
	//id_t parent;
	GameObject* parentObject = nullptr;

	std::string name;
	Device& device;
	id_t id;
};


class GameObjectCamera : public GameObject { 

public:
	GameObjectCamera(id_t id, Device& device, float fov, float aspect_ratio, float nearClip, float farClip)
		: GameObject(id, device), _fov(fov), _aspect_ratio(aspect_ratio), _nearClip(nearClip), _farClip(farClip) { 
		camera = std::make_unique<Camera>(aspect_ratio);
		camera->setPerspectiveProjection(fov, aspect_ratio, nearClip, farClip);
	} 

	std::unique_ptr<Camera> camera = nullptr;

	void debugUI(); 
	void updateCameraView();
	GameObjectType getType() const override { return GameObjectType::CAMERA; } 

private:

	float _fov;
	const float _aspect_ratio;
	const float _nearClip;
	const float _farClip;

	friend class GameObjectFactory;
};

class GameObjectPointLight : public GameObject {

public:
	GameObjectPointLight(id_t id, Device& device, float intencity, float radius, glm::vec3 color = glm::vec3{ 1.f })
		: GameObject(id, device) {
		transform.color = glm::vec4(color, intencity); 
		transform.scale.x = radius; 
	}

	void debugUI();
	GameObjectType getType() const override { return GameObjectType::POINT_LIGHT; }     

	friend class GameObjectFactory;
};

class GameObjectSpotLight : public GameObject {

public:
	GameObjectSpotLight(id_t id, Device& device, float fov, float aspect_ratio, float nearClip, float farClip)
		: GameObject(id, device), _fov(fov), _aspect_ratio(aspect_ratio), _nearClip(nearClip), _farClip(farClip) {

		camera = std::make_unique<Camera>();
		camera->setPerspectiveProjection(fov, aspect_ratio, nearClip, farClip);
	}

	std::unique_ptr<Camera> camera = nullptr;

	void debugUI();

	SpotLight getSpotLightInfo(bool _updateCameraView = false);

	GameObjectType getType() const override { return GameObjectType::SPOT_LIGHT; }

private:
	void updateCameraView();

	float _fov;
	float _aspect_ratio;
	const float _nearClip; 
	const float _farClip;

	friend class GameObjectFactory;
};

class GameObjectModel : public GameObject 
{
public:
	template <typename T>
	void setModel(std::shared_ptr<T> newModel)
	{
		model = std::move(newModel); 
		modelType = static_cast<ModelType>(T::getModelType()); 
		hasModel = true; 
	}

	void setModel(ModelVariant newModel); 
	void setModelType(ModelType type) { modelType = type; } 

	void setMultipleInstances(std::vector<Model::Instance> instances);

	ModelType getModelType() const { return modelType; }

	void createDescriptorSet(DescriptorPool& pool) const; 

	std::vector<VkDescriptorSet> getDescriptorSets() const;

	void update(float dtime);

	void bindModel(VkCommandBuffer& commandBuffer) const;
	void drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex);
	void drawModelDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, int cameraIndex, uint16_t frameIndex);

	void debugUI(); 

	GameObjectType getType() const override { return GameObjectType::MODEL; }

	GameObjectModel(id_t id, Device& device) : GameObject(id, device) { setMultipleInstances({ {} }); }
private:

	bool hasModel = false;
	ModelType modelType = UNDEFINED_MODEL;
	ModelVariant model;

	bool hasMultipleInstances = false;
	std::unique_ptr<Buffer> instancesBuffer = nullptr;
	uint32_t instanceCount = 1;

	int32_t animationIndex{ 0 }; 
	float animationTimer = 0.0f;
	bool animate = false;

	friend class GameObjectFactory; 
};


class GameObjectFactory {

public:
	static GameObject::id_t nextId;

	template <typename T, typename... Args>
	static std::unique_ptr<T> createGameObject(Device& device, Args&&... args) {
		return std::make_unique<T>(nextId++, device, std::forward<Args>(args)...);
	}
};
