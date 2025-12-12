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

ModelLOD ModelUploader::uploadDecodedModel(Device& device, AssetManager& assets, DecodedModel& obj)
{
	std::unique_ptr<Buffer> vertexBuffer = createVertexBuffers(device, obj.vertices.get());
	std::unique_ptr<Buffer> indexBuffer = createIndexBuffers(device, obj.indices);

	//std::vector<Material> materials

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

std::vector<Material> ModelUploader::uploadMaterialsTextures(Device& device, AssetManager& assets, std::vector<DecodedMaterial> materials)
{
	std::vector<Material> outMaterials{};

	for (auto mat : materials) {
		Material targetMat;

		if (!mat.albedoTexture.empty())
		{
			TextureBuilder builder(device);
			targetMat.albedoTexture = assets.textures().create(builder.fromFile(mat.albedoTexture));
		}
		else {
			TextureBuilder builder(device);
			targetMat.albedoTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
		}

		if (!mat.normalTexture.empty())
		{
			TextureBuilder builder(device);
			targetMat.normalTexture = assets.textures().create(builder.fromFile(mat.normalTexture));
		}

		if (!mat.metallicRoughnessTexture.empty())
		{
			TextureBuilder builder(device);
			targetMat.metallicRoughnessTexture = assets.textures().create(builder.fromFile(mat.metallicRoughnessTexture));
		}

		targetMat.metallic = mat.metallic;
		targetMat.roughness = mat.roughness;
		targetMat.baseColorFactor = mat.baseColorFactor;

		outMaterials.push_back(targetMat);
	}

	return outMaterials;
}
