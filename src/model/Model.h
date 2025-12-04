#pragma once

#include <vulkan/vulkan.h>

#include "../base/Device.h"
#include "../base/Buffer.h"

#include "../base/descriptors.h"
#include "../render/Camera.h"

#include "../model/BoundingBox.h"
#include "../textures/TextureObject.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <array>

struct alignas(16) SimplePushConstantData {
	glm::mat4 modelMatrix{ 1.f }; 
	glm::mat4 normalMatrix{ 1.f }; 
};

struct alignas(16) DepthPushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	int indexDepthCamera{ 0 };
};

class Model
{
public:

	static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filePath, const char* filePathTexture);
	static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filePath);
	
	///  model with LOD
	static std::unique_ptr<Model> createModelFromFile(Device& device, std::vector<std::array<std::string, 2>> filesPath);


	struct Instance {
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
	};

	struct Vertex {
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions(bool hasMutipleInstances);
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(bool hasMutipleInstances);
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionsShadow(bool hasMutipleInstances);

		bool operator==(const Vertex& other) const {
			return position == other.position && color == other.color && uv == other.uv;
		}
	};

	struct LodInfo {
		uint32_t vertexOffset; // In vertices
		uint32_t indexOffset;  // In indices
		uint32_t indexCount;   // size of indices used
		size_t   textureIndex = 0; // index of texture used for this LOD
	};


	struct Builder {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		std::vector<LodInfo> lods{};

		BoundingBox aabb;

		bool loadOBJModel(const std::string& filepath);
	};

	Model(Device& device, const Model::Builder& builder);
	~Model(); 

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	void bind(VkCommandBuffer& commandBuffer, bool bindTexture, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, uint16_t modelDescriptorSetIndex, Buffer* instancesBuffer);
	void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, glm::mat4 normalMatrix, const std::array<FrustumPlane, 6>& planes, uint32_t instanceCount);
	void drawDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, uint32_t cameraIndex, const std::array<FrustumPlane, 6>& planes, uint32_t instanceCount);

	// textures should be ordered by lod levels if there are multiple, each lod have the use index 
	void setTexture(std::shared_ptr<TextureObject> newTexture) { texture.resize(1); texture[0] = std::move(newTexture); }
	void setTexture(std::vector<std::shared_ptr<TextureObject>> newTextures) { texture = std::move(newTextures); }
	void addTexture(std::shared_ptr<TextureObject> newTexture) { texture.push_back(std::move(newTexture)); }


	VkDescriptorImageInfo getTextureImageInfo(size_t index = 0) const { return texture[0]->getImageInfo(); }
	
	void createDescriptorSet(DescriptorPool& pool, Device& device);
	std::vector<VkDescriptorSet> getDescriptorSets() { return descriptorSet; };

	bool updateAnimation(uint32_t index, float time) { return false; };
	void update() {};

	static std::vector<DescriptorSetObject> getDescriptorType();
	static const int getModelType() { return 1; }

	BoundingBox getAABB() const { return aabb; }

	std::vector<Model::Instance> instanceList = {};
	std::vector<Model::Instance> getInstanceList() { return instanceList; }

	bool computeShadow = true;

private:
	
	// Axis Aligned Bounding Box
	BoundingBox aabb;
	void createAABB(const std::vector<Vertex>& vertices);

	// LODs
	std::vector<LodInfo> lods{};
	bool hasLODs = false;
	size_t lodIndex = 0;
	void debugValidateLODs() const;

	// Vertex Buffer
	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;
	void createVertexBuffers(const std::vector<Vertex>& vertices);

	// Index Buffer
	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;
	void createIndexBuffers(const std::vector<uint32_t>& indices);

	// Texture and descriptor set
	std::vector<std::shared_ptr<TextureObject>> texture;
	std::vector<VkDescriptorSet> descriptorSet;

	Device& device;

};

