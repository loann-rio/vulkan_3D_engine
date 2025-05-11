#pragma once

#include <vulkan/vulkan.h>
#include "../base/Device.h"
#include "../base/Buffer.h"
#include "../base/Swap_chain.h"
#include "../base/descriptors.h"

#include "../objects/Texture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

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

	struct Builder {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		bool loadOBJModel(const std::string& filepath);
	};

	Model(Device& device, const Model::Builder &builder, const char* filePathTexture = ""); 
	~Model(); 

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filePath, const char* filePathTexture);

	void bind(VkCommandBuffer& commandBuffer, Buffer* instancesBuffer);
	void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 modelMatrix, glm::mat4 normalMatrix, uint32_t instanceCount);
	void drawDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 modelMatrix, uint32_t cameraIndex, uint32_t instanceCount); 

	bool hasTexture = false;
	std::unique_ptr<Texture> texture;
	void setTexture(std::unique_ptr<Texture> newTexture) { texture = std::move(newTexture); }

	
	void createDescriptorSet(DescriptorPool& pool, Device& device);
	std::vector<VkDescriptorSet> getDescriptorSets() { return descriptorSet; };

	bool updateAnimation(uint32_t index, float time) { return false; };
	void update() {};

	static std::vector<DescriptorSetObject> getDescriptorType();
	static int getModelType() { return 1; }

private:
	void createVertexBuffers(const std::vector<Vertex>& vertices);
	void createIndexBuffers(const std::vector<uint32_t>& indices);

	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;

	std::vector<VkDescriptorSet> descriptorSet{ Swap_chain::MAX_FRAMES_IN_FLIGHT };

	static std::vector<VkDescriptorType> bindingDescription;

};

