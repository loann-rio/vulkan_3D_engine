#include "ModelUploader.h"

#include <exception>
#include "../Textures/TextureBuilder.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "../assetManager/AssetManager.h"
#include "../base/Buffer.h"
#include "../base/Device.h"
#include "Decoder/IModelDecoder.h"
#include "ModelAsset.h"
#include "Vertex/IVertexData.h"
#include <vulkan/vulkan_core.h>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
	TextureManager::TextureID createTexture(Device& device, AssetManager& assets, const DecodedTexture& texture)
	{
		TextureBuilder builder(device);

		// embedded image, already decoded to RGBA8 by the decoder
		if (!texture.pixels.empty()) {
			uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture.width, texture.height)))) + 1;
			return assets.textures().create(builder.fromCharBuffer(texture.pixels, texture.width, texture.height, 4, mipLevels));
		}

		return assets.textures().create(builder.fromFile(texture.path));
	}
}

ModelLOD ModelUploader::uploadDecodedModel(Device& device, AssetManager& assets, DecodedModel& obj)
{
	std::unique_ptr<Buffer> vertexBuffer = createVertexBuffers(device, obj.vertices.get());
	std::unique_ptr<Buffer> indexBuffer = createIndexBuffers(device, obj.indices);

	ModelLOD asset{};
	asset.vertexBuffer = std::move(vertexBuffer);
	asset.vertexCount = obj.vertices->vertexCount();
	asset.indexBuffer = std::move(indexBuffer);
	asset.indexCount = obj.indices.size();
	asset.vertexStride = obj.vertices->stride();
	asset.primitives = obj.primitives;
	asset.materials = uploadMaterialsTextures(device, assets, obj.materials);

	return asset;
}

std::unique_ptr<Buffer> ModelUploader::createVertexBuffers(Device& device, IVertexData* vertices)
{
	uint32_t vertexCount = vertices->vertexCount();

	if (vertexCount < 3) {
		throw std::exception("model uploader : Vertex count must be at least 3");
	}

	VkDeviceSize bufferSize = vertices->layout().stride() * vertexCount;
	uint32_t vertexSize = vertices->layout().stride();

	Buffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices->rawData());

	std::unique_ptr<Buffer> vertexBuffer = std::make_unique<Buffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);

	return std::move(vertexBuffer);
}

std::unique_ptr<Buffer> ModelUploader::createIndexBuffers(Device& device, const std::vector<uint32_t>& indices)
{
	uint32_t indexCount = static_cast<uint32_t>(indices.size());

	if (indexCount < 0) {
		throw std::exception("model uploader : index buffer cannot be empty");
	}

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

	std::unique_ptr<Buffer> indexBuffer = std::make_unique<Buffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);

	return std::move(indexBuffer);
}

void ModelUploader::uploadShaderMaterials(Device& device, ModelLOD& lod)
{
	lod.textures.clear();

	// returns the index of the texture in lod.textures, -1 if the material doesn't use it
	auto textureIndex = [&lod](TextureManager::TextureID id) -> int {
		if (!id) return -1;

		auto it = std::find(lod.textures.begin(), lod.textures.end(), id);
		if (it != lod.textures.end()) return static_cast<int>(it - lod.textures.begin());

		if (lod.textures.size() >= MAX_MODEL_TEXTURES) {
			std::cerr << "ModelUploader: more than " << MAX_MODEL_TEXTURES << " textures in model, texture ignored\n";
			return -1;
		}

		lod.textures.push_back(id);
		return static_cast<int>(lod.textures.size() - 1);
	};

	std::vector<ShaderMaterialData> shaderMaterials{};
	shaderMaterials.reserve(lod.materials.size());

	for (const auto& material : lod.materials) {
		ShaderMaterialData data{};

		data.baseColorFactor = material.baseColorFactor;
		data.emissiveFactor = glm::vec4(material.emissiveFactor, 1.f);
		data.metallicFactor = material.metallic;
		data.roughnessFactor = material.roughness;
		data.alphaMask = material.alphaMask ? 1.f : 0.f;
		data.alphaMaskCutoff = material.alphaCutoff;

		data.baseColorTextureIndex = textureIndex(material.albedoTexture);
		data.metallicRoughnessTextureIndex = textureIndex(material.metallicRoughnessTexture);
		data.normalTextureIndex = textureIndex(material.normalTexture);
		data.occlusionTextureIndex = textureIndex(material.occlusionTexture);
		data.emissiveTextureIndex = textureIndex(material.emissiveTexture);

		// only TEXCOORD_0 is decoded for now
		data.baseColorTextureSet = data.baseColorTextureIndex > -1 ? 0 : -1;
		data.physicalDescriptorTextureSet = data.metallicRoughnessTextureIndex > -1 ? 0 : -1;
		data.normalTextureSet = data.normalTextureIndex > -1 ? 0 : -1;
		data.occlusionTextureSet = data.occlusionTextureIndex > -1 ? 0 : -1;
		data.emissiveTextureSet = data.emissiveTextureIndex > -1 ? 0 : -1;

		shaderMaterials.push_back(data);
	}

	if (shaderMaterials.empty())
		shaderMaterials.push_back(ShaderMaterialData{});

	uint32_t instanceSize = sizeof(ShaderMaterialData);
	uint32_t instanceCount = static_cast<uint32_t>(shaderMaterials.size());
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(instanceSize) * instanceCount;

	Buffer stagingBuffer{
		device,
		instanceSize,
		instanceCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)shaderMaterials.data());

	lod.materialBuffer = std::make_unique<Buffer>(
		device,
		instanceSize,
		instanceCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), lod.materialBuffer->getBuffer(), bufferSize);
}

std::vector<Material> ModelUploader::uploadMaterialsTextures(Device& device, AssetManager& assets, std::vector<DecodedMaterial> materials)
{
	std::vector<Material> outMaterials{};

	for (auto mat : materials) {
		Material targetMat;

		if (!mat.albedoTexture.empty())
			targetMat.albedoTexture = createTexture(device, assets, mat.albedoTexture);
		else
			targetMat.albedoTexture = createTexture(device, assets, DecodedTexture("assets/textures/whiteTexture.jpg"));

		if (!mat.normalTexture.empty())
			targetMat.normalTexture = createTexture(device, assets, mat.normalTexture);

		if (!mat.metallicRoughnessTexture.empty())
			targetMat.metallicRoughnessTexture = createTexture(device, assets, mat.metallicRoughnessTexture);

		if (!mat.occlusionTexture.empty())
			targetMat.occlusionTexture = createTexture(device, assets, mat.occlusionTexture);

		if (!mat.emissiveTexture.empty())
			targetMat.emissiveTexture = createTexture(device, assets, mat.emissiveTexture);

		targetMat.metallic = mat.metallic;
		targetMat.roughness = mat.roughness;
		targetMat.baseColorFactor = mat.baseColorFactor;
		targetMat.emissiveFactor = mat.emissiveFactor;
		targetMat.alphaMask = mat.alphaMask;
		targetMat.alphaCutoff = mat.alphaCutoff;

		outMaterials.push_back(targetMat);
	}

	return outMaterials;
}
