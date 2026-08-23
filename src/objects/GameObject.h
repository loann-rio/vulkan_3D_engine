#pragma once

#include "../base/Swap_chain.h"
#include "../base/descriptors.h"
#include "../base/Device.h"
#include "../model/GlTFModel.h"
#include "../model/Model.h"
#include "../render/Camera.h"
#include "../assetManager/AssetManager.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

#include "../model/ModelAsset.h"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <iostream>
#include <type_traits>
#include <variant>
#include <unordered_map>

#define REGISTER_BEHAVIOR(T)                                \
    std::string getClassName() const override { return #T; }\
    struct T##Registrator {                                 \
        T##Registrator() {                                  \
            GameObjectBehavior::registerBehavior(           \
                #T,                                         \
                [](Device& device) {                        \
                    return std::make_unique<T>(device);     \
                }                                           \
            );                                              \
        }                                                   \
    };                                                      \
    inline static T##Registrator T##RegistratorInstance;



class ObjectManager;
class GameObject;
class GameObjectBehavior;

using ModelVariant = std::variant<std::shared_ptr<Model>,
	std::shared_ptr<GlTFModel::ModelGltf>>;

enum class ModelType {
	UNDEFINED_MODEL = 0,
	OBJ_MODEL = 1,
	GLTF_MODEL = 2,
	PREBUILT_MODEL = 3,
	
};

enum class PrimitivesModelType {
	NONE = 0,
	PLANE = 1,
	CUBE = 2,
	SPHERE = 3,
	CYLINDER = 4,
	CONE = 5,
};

enum class ModelSubType { 
	NONE = 0,
	TERRAIN = 1,
	SKYBOX = 2,
};

enum class GameObjectType { 
	UNKNOWN = 0,
	CAMERA = 1,
	MODEL = 2,
	SPOT_LIGHT = 3,
	POINT_LIGHT = 4
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

class GameObjectBehavior { 
public:
	virtual ~GameObjectBehavior() = default;

	virtual std::string getClassName() const = 0;

	virtual void setup(Device& device, ObjectManager* objManager, GameObject* object) = 0;
	virtual void loop(Device& device, ObjectManager* objManager, GameObject* object) = 0;

	// Factory signature
	using CreatorFn = std::function<std::unique_ptr<GameObjectBehavior>(Device&)>;

	// Register a behavior by name
	static void registerBehavior(const std::string& name, CreatorFn fn) {
		getRegistry()[name] = std::move(fn);
	}

	// Factory method
	static std::unique_ptr<GameObjectBehavior> createBehaviorFromType(const std::string& name, Device& device) {
		auto it = getRegistry().find(name);
		if (it != getRegistry().end()) {
			return it->second(device);
		}
		return nullptr; // unknown behavior type
	}

private:
	static std::unordered_map<std::string, CreatorFn>& getRegistry() {
		static std::unordered_map<std::string, CreatorFn> registry;
		return registry;
	}
}; 

class GameObject
{
public:

	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, std::unique_ptr<GameObject>>;

	virtual GameObjectType getType() const { return GameObjectType::UNKNOWN; }
	id_t getId() { return id; } 

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	GameObject(GameObject&&) = default; 
	GameObject& operator=(GameObject&&) = default;

	GameObject(id_t id, Device& device, AssetManager& assets) : id(id), device(device), assets(assets) {}
	virtual ~GameObject() = default;

	// UI
	virtual void debugUI();
	
	// position scale rotation
	TransformComponent transform{};

	// name
	void setName(std::string newName) { name = newName; }
	std::string getName() const;

	// parent child
	void setParent(GameObject* parent) { parentObject = parent; }
	void setChild(GameObject* child) { assert(this != child); child->setParent(this); } 

	// matrices
	glm::mat4 getTransformMat();
	glm::mat3 getNormalMat();

	// attached behavior class 
	bool hasAttachedClass = false; 

	std::string getAttachedClassType() const {
		if (!hasAttachedClass) return std::string{};
		return attachedClass->getClassName();
	}

	void setAttachedClass(std::unique_ptr<GameObjectBehavior> attClass) { attachedClass = std::move(attClass); hasAttachedClass = true;} 
	void setup(ObjectManager* objManager) { if (hasAttachedClass) attachedClass->setup(device, objManager, this); } 
	void loop(ObjectManager* objManager) { if (hasAttachedClass) attachedClass->loop(device, objManager, this); }
	
	// mark for removal
	bool toBeRemoved = false;
	bool saveable = true;

protected:
   
	std::unique_ptr<GameObjectBehavior> attachedClass = nullptr;

	GameObject* parentObject = nullptr;

	std::string name;
	Device& device;
	AssetManager& assets;
	id_t id;

	friend class GameObjectBehavior;
};

class GameObjectCamera : public GameObject { 

public:
	GameObjectCamera(id_t id, Device& device, AssetManager& assets, float fov, float aspect_ratio, float nearClip, float farClip)
		: GameObject(id, device, assets), _fov(fov), _aspect_ratio(aspect_ratio), _nearClip(nearClip), _farClip(farClip) { 
		camera = std::make_unique<Camera>(aspect_ratio);
		camera->setPerspectiveProjection(fov, aspect_ratio, nearClip, farClip);
	} 

	std::unique_ptr<Camera> camera = nullptr;

	void debugUI(); 
	void updateCameraView();
	GameObjectType getType() const override { return GameObjectType::CAMERA; } 

	float getFov() const { return _fov; }
	float getAspectRatio() const { return _aspect_ratio; }
	float getNearClip() const { return _nearClip; }
	float getFarClip() const { return _farClip; }
	std::array<FrustumPlane, 6> getFrustumPlanes() const { return camera->getFrustum(); }

private:

	float _fov;
	const float _aspect_ratio;
	const float _nearClip;
	const float _farClip;

	friend class GameObjectFactory;
};

class GameObjectPointLight : public GameObject {

public:
	GameObjectPointLight(id_t id, Device& device, AssetManager& assets, float intencity, float radius, glm::vec3 color = glm::vec3{ 1.f })
		: GameObject(id, device, assets) {
		transform.color = glm::vec4(color, intencity); 
		transform.scale.x = radius; 
	}

	void debugUI();
	GameObjectType getType() const override { return GameObjectType::POINT_LIGHT; }     

	friend class GameObjectFactory;
};

class GameObjectSpotLight : public GameObject {

public:
	GameObjectSpotLight(id_t id, Device& device, AssetManager& assets, float fov, float aspect_ratio, float nearClip, float farClip)
		: GameObject(id, device, assets), _fov(fov), _aspect_ratio(aspect_ratio), _nearClip(nearClip), _farClip(farClip) {

		camera = std::make_unique<Camera>();
		camera->setPerspectiveProjection(fov, aspect_ratio, nearClip, farClip);
	}

	std::unique_ptr<Camera> camera = nullptr;

	void debugUI();

	SpotLight getSpotLightInfo(bool _updateCameraView = false);

	GameObjectType getType() const override { return GameObjectType::SPOT_LIGHT; }

	float getFov() const { return _fov; }
	float getAspectRatio() const { return _aspect_ratio; }
	float getNearClip() const { return _nearClip; }
	float getFarClip() const { return _farClip; }

	std::array<FrustumPlane, 6> getFrustumPlanes() const { return camera->getFrustum(); }

private:
	void updateCameraView();

	float _fov; // field of view
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

	void setModel(std::shared_ptr<Model> model);
	void setModel(std::shared_ptr<GlTFModel::ModelGltf> model);
	void setModel(ModelVariant newModel);


	void setModel(ModelManager::ModelID _model) {
		modelAsset = _model;
		modelType = ModelType::OBJ_MODEL;
		hasModel = true;
	}


	// setters getters
	void setModelType(ModelType type) { modelType = type; } 
	ModelType getModelType() const { return modelType; } 

	void setModelSubType(ModelSubType type) { modelSubType = type; }
	ModelSubType getModelSubType() const { return modelSubType; }

	void setPrimitivesModelType(PrimitivesModelType type) { primitivesModelType = type; }
	PrimitivesModelType getPrimitivesModelType() const { return primitivesModelType; }

	VkDescriptorImageInfo getTextureImageInfo() const;
	std::vector<VkDescriptorSet> getDescriptorSets() const;

	void setMultipleInstances(std::vector<Model::Instance> instances);

	void createDescriptorSet(DescriptorPool& pool) const; 

	BoundingBox getAABB() const;

	void update(float dtime);

	void bindModel(VkCommandBuffer& commandBuffer, bool bindTexture, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, uint16_t modelDescriptorSetIndex) const;
	void drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, const std::array<FrustumPlane, 6>& frustrumPlanes);
	void drawModelDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, int cameraIndex, uint16_t frameIndex, const std::array<FrustumPlane, 6>& planes);

	void debugUI(); 

	//std::unique_ptr<ModelAsset> modelAsset;
	ModelManager::ModelID modelAsset;

	bool show = true;

	GameObjectType getType() const override { return GameObjectType::MODEL; }
	std::string texturePath;
	std::string modelPath;

	// primitive info
	int primitiveLOD = 0;

	GameObjectModel(id_t id, Device& device, AssetManager& assets) : GameObject(id, device, assets) { setMultipleInstances({ {} }); }
private:

	bool hasModel = false;

	ModelType modelType = ModelType::UNDEFINED_MODEL;
	ModelSubType modelSubType = ModelSubType::NONE;
	PrimitivesModelType primitivesModelType = PrimitivesModelType::NONE;
	
	ModelVariant model;

	std::unique_ptr<Buffer> instancesBuffer = nullptr;
	std::vector<std::unique_ptr<Buffer>> frameInstancesBuffer;
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
	static std::unique_ptr<T> createGameObject(Device& device, AssetManager& assets, Args&&... args) {
		return std::make_unique<T>(nextId++, device, assets, std::forward<Args>(args)...);
	}
};
