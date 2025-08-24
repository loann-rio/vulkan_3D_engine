#include "Model.h"

#include "../base/Utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../external/tinyobjectloader/tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cstring>
#include <cassert>
#include <iostream>
#include <unordered_map>

namespace std {
	template<>
	struct hash<Model::Vertex>
	{
		size_t operator()(Model::Vertex const& vertex) const {
			size_t seed = 0;
			hashCombine(seed, vertex.position, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

Model::Model(Device& device, const Model::Builder& builder, const char* filePathTexture) : device{ device } {
	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);

	if (filePathTexture) {
		texture = Texture::create(device, filePathTexture); 
		if (texture != nullptr)
			hasTexture = true;
	}
}

Model::~Model() {}

std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filePath, const char* filePathTexture = "textures\\whiteTexture.jpg")
{
	Builder builder{}; 
	if (builder.loadOBJModel(filePath)) { 
		std::cout << "vertex count: " << builder.vertices.size() << "\n"; 
		std::unique_ptr<Model> m = std::make_unique<Model>(device, builder, filePathTexture); 
		return m;
	}

	return nullptr;
}

void Model::bind(VkCommandBuffer& commandBuffer, Buffer* instancesBuffer) 
{
	
	VkBuffer buffers[] = { vertexBuffer->getBuffer(), instancesBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0, 0 }; 
	vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
	
	if (hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& PipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, glm::mat4 normalMatrix, uint32_t instanceCount = 1)
{
	SimplePushConstantData push{};  
	push.modelMatrix = modelMatrix;  
	push.normalMatrix = normalMatrix; 

	vkCmdPushConstants( 
		commandBuffer, 
		PipelineLayout,  
		VK_SHADER_STAGE_VERTEX_BIT, 
		0,
		sizeof(push),  
		&push 
	);

	uint32_t firstInstance = (instanceCount == 1) ? 0 : 1;

	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, firstInstance);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, firstInstance);
	}
}

void Model::drawDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, uint32_t cameraIndex, uint32_t instanceCount)
{
	DepthPushConstantData push{};   
	push.modelMatrix = modelMatrix; 
	push.indexDepthCamera = cameraIndex; 

	vkCmdPushConstants(		
		commandBuffer, 
		pipelineLayout, 
		VK_SHADER_STAGE_VERTEX_BIT, 
		0,
		sizeof(push), 
		&push 
	);

	uint32_t firstInstance = (instanceCount == 1) ? 0 : 1;

	if (hasIndexBuffer) { 
		vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, firstInstance);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, firstInstance);
	}
}

void Model::createDescriptorSet(DescriptorPool& pool, Device& device)
{
	auto textureSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	for (int i = 0; i < descriptorSet.size(); i++)
	{
		auto imageInfo = texture->getImageInfo();
		DescriptorWriter(*textureSetLayout, pool)
			.writeImage(1, &imageInfo)
			.build(descriptorSet[i]);
	}
}

std::vector<DescriptorSetObject> Model::getDescriptorType()
{
	std::vector<DescriptorObject> set1 = {
		 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1} 
	};

	return std::vector<DescriptorSetObject>{{set1, 2}};
}

void Model::createVertexBuffers(const std::vector<Vertex>& vertices)
{
	vertexCount = static_cast<uint32_t>(vertices.size());

	assert(vertexCount >= 3 && "Vertex count must be at least 3");

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

	uint32_t vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	vertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Model::createIndexBuffers(const std::vector<uint32_t>& indices)
{
	indexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = indexCount > 0;

	if (!hasIndexBuffer) return;

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	Buffer stagingBuffer{
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions(bool hasMutipleInstances = false)
{
	std::vector<VkVertexInputBindingDescription> bindingDescription(1);
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	//if (hasMutipleInstances)
		bindingDescription.push_back({ 1, sizeof(Instance), VK_VERTEX_INPUT_RATE_INSTANCE });
		
	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions(bool hasMutipleInstances = false)
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
	attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
	attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
	attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

	attributeDescriptions.push_back({ 4, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, position) });
	attributeDescriptions.push_back({ 5, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, rotation) });
	attributeDescriptions.push_back({ 6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, scale) });

	return attributeDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptionsShadow(bool hasMutipleInstances = false)
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });

	if (hasMutipleInstances) {
		attributeDescriptions.push_back({ 1, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, position) }); 
		attributeDescriptions.push_back({ 2, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, rotation) }); 
		attributeDescriptions.push_back({ 3, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, scale) }); 
	}
	
	return attributeDescriptions;
}

bool Model::Builder::loadOBJModel(const std::string& filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
		//throw std::runtime_error(warn + err);
		std::cerr << warn + err << "\n";
		return false;
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			if (index.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				vertex.color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2],
				};
			}

			if (index.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (index.texcoord_index >= 0) {
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return true;
}
