#pragma once

#include <vulkan/vulkan.h>
#include "Device.h"
#include "Buffer.h"
#include "Texture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

class Model
{
public:

	struct Vertex {
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex& other) const {
			return position == other.position && color == other.color && uv == other.uv;
		}
	};

	struct Builder {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void loadModel(const std::string& filepath); 
		void loadOBJModel(const std::string& filepath);
	};

	Model(Device& device, const Model::Builder &builder, const char* filePathTexture); 
	~Model(); 

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filePath, const char* filePathTexture);

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

	bool hasTexture = false;
	//VkDescriptorImageInfo getImageInfo();

	std::unique_ptr<Texture> texture;
	
private:
	void createVertexBuffers(const std::vector<Vertex>& vertices);
	void createIndexBuffers(const std::vector<uint32_t>& indices);

	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;

};

