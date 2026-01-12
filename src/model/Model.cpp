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
#include "../Textures/TextureBuilder.h"
#include "../base/Swap_chain.h"

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

std::unique_ptr<Model> Model::createModelFromFile(Device& device, AssetManager& assets, const std::string& filePath, const char* filePathTexture)
{
	Builder builder{};
	if (builder.loadOBJModel(filePath)) {
		std::unique_ptr<Model> m = std::make_unique<Model>(device, assets, builder); 

		TextureBuilder builder(device);

		if (m) m->setTexture(assets.textures().create(builder.fromFile(filePathTexture)));
		if (m) return m;
	}

	return nullptr;
}

std::unique_ptr<Model> Model::createModelFromFile(Device& device, AssetManager& assets, const std::string& filePath)
{
	Builder builder{};
	if (builder.loadOBJModel(filePath)) {
		std::unique_ptr<Model> m = std::make_unique<Model>(device, assets, builder);
		if (m) return m;
	}
	return nullptr;
}

std::unique_ptr<Model> Model::createModelFromFile(
	Device& device, AssetManager& assets,
	std::vector<std::array<std::string, 2>> filesPath)
{
	if (filesPath.empty()) {
		return nullptr;
	}

	if (filesPath[0][1].empty()) {
		std::cerr << "Model::createModelFromFile() Warning: First LOD texture path should always be defined!" << std::endl;
		return nullptr;
	}

	Builder builder{};

	size_t textureCount = -1;
	std::vector<TextureManager::TextureID> textures;

	for (auto& filePath : filesPath)
	{
		// record global offsets BEFORE loading
		uint32_t vertexOffsetBefore = static_cast<uint32_t>(builder.vertices.size());
		uint32_t indexOffsetBefore = static_cast<uint32_t>(builder.indices.size());

		// Load OBJ (fills builder.vertices + builder.indices)
		if (!builder.loadOBJModel(filePath[0]))
			continue;

		// record AFTER sizes
		uint32_t vertexOffsetAfter = static_cast<uint32_t>(builder.vertices.size());
		uint32_t indexOffsetAfter = static_cast<uint32_t>(builder.indices.size());

		uint32_t newVertexCount = vertexOffsetAfter - vertexOffsetBefore;
		uint32_t newIndexCount = indexOffsetAfter - indexOffsetBefore;

		if (newVertexCount == 0 || newIndexCount == 0) {
			// no geometry loaded for this file — skip it
			continue;
		}

		// LOD description
		LodInfo lod{};
		lod.vertexOffset = vertexOffsetBefore;    // start of this LOD vertices
		lod.indexOffset = indexOffsetBefore;      // start of this LOD indices
		lod.indexCount = newIndexCount;           // number of indices in this LOD

		lod.textureIndex = textureCount;

		if (!filePath[1].empty()) {
			// Load texture for this LOD

			TextureBuilder builder(device);
			auto texture = assets.textures().create(builder.fromFile(filePath[1].c_str()));

			if (texture != 0) {
				textures.push_back(texture);
				textureCount++;
				lod.textureIndex = textureCount;
			}
		}

		builder.lods.push_back(lod);
	}

	if (builder.vertices.empty() || builder.indices.empty()) {
		return nullptr;
	}

	auto model = std::make_unique<Model>(device, assets, builder);
	model->setTexture(textures);
	model->hasLODs = true;

	return model;
}


Model::Model(Device& device, AssetManager& assets, const Model::Builder& builder) : device{ device }, aabb{ builder.aabb }, assets{ assets }
{
	if (builder.aabb.valid == false) {
		createAABB(builder.vertices);
	}

	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);

	if (builder.lods.size() > 0) {
		lods = builder.lods;
		hasLODs = true;
	}

	//debugValidateLODs();
	
	

}

Model::~Model() {
	for (auto textureID : textures) {
		assets.textures().remove(textureID);
	}
}


void Model::bind(VkCommandBuffer& commandBuffer, bool bindTexture, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, uint16_t modelDescriptorSetIndex, Buffer* instancesBuffer)
{
	if (bindTexture)
	{
		int descriptorSetIndex = frameIndex;
		if (descriptorSet.size() > 0)
		{
			if (hasLODs) descriptorSetIndex += Swap_chain::MAX_FRAMES_IN_FLIGHT * lods[lodIndex].textureIndex;

			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				modelDescriptorSetIndex, 1,
				&descriptorSet[descriptorSetIndex],
				0,
				nullptr);
		}
		else
			{
			throw std::runtime_error("Model::bind() failed to bind descriptor set: no descriptor set available");
		}
	}

	VkBuffer buffers[] = { vertexBuffer->getBuffer(), instancesBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0, 0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
	
	if (hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& PipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, glm::mat4 normalMatrix, const std::array<FrustumPlane, 6>& planes, uint32_t instanceCount = 1)
{
	if (!Camera::isAABBinFrustrum(aabb.getAABB(modelMatrix), planes)) return;

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

	// if multiple instance skip reference instance
	//instanceCount = instanceList.size() + 1;
	uint32_t firstInstance = (instanceCount == 1) ? 0 : 1;

	uint32_t drawIndexCount = indexCount;
	uint32_t firstIndex = 0;
	int32_t  vertexOffsetParam = 0;

	if (hasLODs) {
		drawIndexCount = lods[lodIndex].indexCount;
		firstIndex = lods[lodIndex].indexOffset;
	}

	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, drawIndexCount, instanceCount, firstIndex, 0, firstInstance);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, firstInstance);
	}
}

void Model::drawDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frameIndex, glm::mat4 modelMatrix, uint32_t cameraIndex, const std::array<FrustumPlane, 6>& planes, uint32_t instanceCount) 
{
	if (!Camera::isAABBinFrustrum(aabb.getAABB(modelMatrix), planes)) return;
	if (!computeShadow) return;

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

	uint32_t drawIndexCount = indexCount;
	uint32_t firstIndex = 0;
	int32_t  vertexOffsetParam = 0;

	if (hasLODs) {
		drawIndexCount = lods[lodIndex].indexCount;
		firstIndex = lods[lodIndex].indexOffset;
	}

	if (hasIndexBuffer) { 
		vkCmdDrawIndexed(commandBuffer, drawIndexCount, instanceCount, firstIndex, 0, firstInstance);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, firstInstance);
	}
}

void Model::createDescriptorSet(DescriptorPool& pool, Device& device)
{

	auto textureSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	descriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT * textures.size());
	for (int i = 0; i < descriptorSet.size(); i++)
	{
		auto imageInfo = assets.textures().get(textures[int(i/2)])->getImageInfo();
		DescriptorWriter(*textureSetLayout, pool)
			.writeImage(0, &imageInfo)
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

void Model::createAABB(const std::vector<Vertex>& vertices)
{
	if (vertices.size() == 0) {
		aabb.valid = false;
		return;
	}

	aabb.min = vertices[0].position;
	aabb.max = vertices[0].position;

	for (const auto& vertex : vertices) {
		aabb.min.x = std::min(aabb.min.x, vertex.position.x);
		aabb.min.y = std::min(aabb.min.y, vertex.position.y);
		aabb.min.z = std::min(aabb.min.z, vertex.position.z);

		aabb.max.x = std::max(aabb.max.x, vertex.position.x);
		aabb.max.y = std::max(aabb.max.y, vertex.position.y);
		aabb.max.z = std::max(aabb.max.z, vertex.position.z);
	}
	aabb.valid = true;
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
	attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT   , offsetof(Vertex, uv) });

	if (hasMutipleInstances) {
		attributeDescriptions.push_back({ 4, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, position) });
		attributeDescriptions.push_back({ 5, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, rotation) });
		attributeDescriptions.push_back({ 6, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance, scale) });
	}

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
		std::cerr << warn + err << "\n";
		return false;
	}

	std::vector<Vertex> localVertices;
	std::vector<uint32_t> localIndices;
	std::unordered_map<Vertex, uint32_t> uniqueLocalVertices;

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

				if (!attrib.colors.empty() && (size_t)(3 * index.vertex_index + 2) < attrib.colors.size()) {
					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}
				else {
					vertex.color = { 1.0f, 1.0f, 1.0f }; // default color
				}
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

			auto it = uniqueLocalVertices.find(vertex);
			if (it == uniqueLocalVertices.end()) {
				uint32_t newIndex = static_cast<uint32_t>(localVertices.size());
				uniqueLocalVertices.emplace(vertex, newIndex);
				localVertices.push_back(vertex);
				localIndices.push_back(newIndex);
			}
			else {
				localIndices.push_back(it->second);
			}
		}
	}

	if (localVertices.empty() || localIndices.empty()) {
		return false;
	}

	// Append local geometry to global builder arrays
	uint32_t baseVertex = static_cast<uint32_t>(vertices.size()); // global base for this LOD

	// update global AABB using the local vertices (transform indices to global positions)
	for (const auto& v : localVertices) {
		if (!aabb.valid) {
			aabb.min = v.position;
			aabb.max = v.position;
			aabb.valid = true;
		}
		else {
			aabb.min.x = std::min(aabb.min.x, v.position.x);
			aabb.min.y = std::min(aabb.min.y, v.position.y);
			aabb.min.z = std::min(aabb.min.z, v.position.z);

			aabb.max.x = std::max(aabb.max.x, v.position.x);
			aabb.max.y = std::max(aabb.max.y, v.position.y);
			aabb.max.z = std::max(aabb.max.z, v.position.z);
		}
	}

	// append local vertices
	vertices.insert(vertices.end(), localVertices.begin(), localVertices.end());

	// append local indices adjusted by baseVertex (to point into global vertex buffer)
	for (uint32_t localIdx : localIndices) {
		indices.push_back(baseVertex + localIdx);
	}

	return true;
}

void Model::debugValidateLODs() const {
#ifndef NDEBUG
	if (!indexBuffer) return;
	VkDeviceSize indexBufferSizeBytes = indexBuffer->getBufferSize();
	for (size_t i = 0; i < lods.size(); ++i) {
		const LodInfo& l = lods[i];
		VkDeviceSize startByte = VkDeviceSize(l.indexOffset) * sizeof(uint32_t);
		VkDeviceSize endByte = startByte + VkDeviceSize(l.indexCount) * sizeof(uint32_t);

		std::cerr << "LOD " << i << " : indexOffset=" << l.indexOffset
			<< " indexCount=" << l.indexCount
			<< " byteRange=[" << startByte << "," << endByte << ")"
			<< " indexBufferSize=" << indexBufferSizeBytes << "\n";

		assert(endByte <= indexBufferSizeBytes && "LOD index range exceeds index buffer size!");
	}
#endif
}
