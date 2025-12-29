#include "ModelUploader.h"

#include <exception>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

#include "../assetManager/AssetManager.h"
#include "../Textures/TextureBuilder.h"
#include "../base/Buffer.h"
#include "../base/Device.h"

#include "ModelNode.h"

#include "Decoder/IModelDecoder.h"
#include "ModelAsset.h"

#include <vulkan/vulkan_core.h>

namespace {
	
	void nodeIndicesToPtrs(std::vector<std::unique_ptr<Node>>& linearNodes, std::vector<Node*>& target, std::vector<size_t>& source) 
	{
		for (size_t nodeIndex : source) 
		{
			target.push_back(
				linearNodes[nodeIndex].get());
		}
	}

	void populateRootNodes(ModelLOD& model, std::vector<size_t>& rootIndices) 
	{
		if (rootIndices.empty()) {
			throw std::runtime_error("Failed to populate root node : model must have roots");
		}

		nodeIndicesToPtrs(model.LinearNodes, model.nodes, rootIndices);
	}		

	std::vector<std::unique_ptr<Skin>> uploadSkins(ModelLOD& model, std::vector<DecodedSkin>& skins) 
	{
		std::vector<std::unique_ptr<Skin>> resultSkins{};

		for (DecodedSkin& skin : skins) {
			auto newSkin = std::make_unique<Skin>();

			newSkin->name = skin.name;

			newSkin->inverseBindMatrices = std::move(skin.inverseBindMatrices);
			
			if (skin.skeletonRootIndex >= 0 && static_cast<size_t>(skin.skeletonRootIndex) < model.LinearNodes.size()) {
				newSkin->skeletonRoot = model.LinearNodes[skin.skeletonRootIndex].get();
			}
			else {
				newSkin->skeletonRoot = nullptr;
			}

			std::vector<size_t> jointIdxs;
			jointIdxs.reserve(skin.jointsIndex.size());
			for (int ji : skin.jointsIndex) {
				if (ji >= 0 && static_cast<size_t>(ji) < model.LinearNodes.size())
					jointIdxs.push_back(static_cast<size_t>(ji));
			}

			nodeIndicesToPtrs(model.LinearNodes, newSkin->joints, jointIdxs);

			resultSkins.push_back(std::move(newSkin));
		}

		return resultSkins;
	}

	void assignSkinToNodes(Device& device, std::vector<std::unique_ptr<Node>>& linearNodes, std::vector<std::unique_ptr<Skin>>& skins)
	{
		for (auto& node : linearNodes) {
			// Assign skins
			if (node->skinIndex > -1) {
				node->hasSkin = true;
				node->skin = skins[node->skinIndex].get();
			}

			node->createBuffer(node->hasSkin, device);

			// Initial pose
			node->update_cpu();
		}
	}

	AnimationSampler uploadSampler(DecodedAnimationSampler& sampler) {
		AnimationSampler newSampler{};
		newSampler.interpolation = 
			static_cast<AnimationSampler::InterpolationType>(sampler.interpolation);

		newSampler.inputs = std::move(sampler.inputs);
		newSampler.outputsVec4 = std::move(sampler.outputsVec4);
		newSampler.outputs = std::move(sampler.outputs);

		return newSampler;
	}

	AnimationChannel uploadChannel(ModelLOD& model, DecodedAnimationChannel& channel) {
		AnimationChannel new_channel{};

		new_channel.path =
			static_cast<AnimationChannel::PathType>(channel.path);

		new_channel.samplerIndex = channel.samplerIndex;

		new_channel.node = model.LinearNodes[channel.nodeIndex].get();

		return new_channel;
	}

	std::vector<Animation> uploadAnimation(ModelLOD& model, std::vector<DecodedAnimation>& animations)
	{
		std::vector<Animation> targetAnimations{};

		for (DecodedAnimation& anim : animations) 
		{
			Animation newAnimation{};

			newAnimation.name = anim.name;

			newAnimation.start = anim.start;
			newAnimation.end = anim.end;

			newAnimation.samplers.reserve(anim.samplers.size());
			for (DecodedAnimationSampler sampler : anim.samplers) {
				newAnimation.samplers.push_back(uploadSampler(sampler));
			}

			newAnimation.channels.reserve(anim.channels.size());
			for (DecodedAnimationChannel channel : anim.channels) {
				newAnimation.channels.push_back(uploadChannel(model, channel));
			}
		}

		return targetAnimations;
	}
}

ModelLOD ModelUploader::uploadDecodedModel(Device& device, AssetManager& assets, DecodedModel& obj)
{
	ModelLOD asset{};

	asset.LinearNodes = std::move(obj.nodes);
	populateRootNodes(asset, obj.rootNodes);

	asset.skins = uploadSkins(asset, obj.skins);
	assignSkinToNodes(device, asset.LinearNodes, asset.skins);
	
	asset.vertexBuffer = createVertexBuffers(device, obj.vertices.get());
	asset.indexBuffer = createIndexBuffers(device, obj.indices);

	asset.vertexCount = obj.vertices->vertexCount();
	asset.indexCount = obj.indices.size();

	asset.vertexStride = obj.vertices->stride();
	
	const auto textureList = uplaoadTextureList(device, assets, obj.textures);
	asset.materials = uploadMaterialsTextures(device, assets, obj.materials, textureList);

	if (obj.aabb.valid) {
		asset.aabb = obj.aabb;
	}

	asset.animations = uploadAnimation(asset, obj.animations);

	for (auto& nodePtr : asset.LinearNodes) {
		Node* node = nodePtr.get();

		// childrenIndices is vector<int>, iterate as int and validate
		for (int childrenIndex : node->childrenIndices) {
			if (childrenIndex >= 0 && static_cast<size_t>(childrenIndex) < asset.LinearNodes.size()) {
				node->children.push_back(asset.LinearNodes[static_cast<size_t>(childrenIndex)].get());
			}
		}

		// fix parent assignment: check bounds and non-negative index
		if (node->parentIndex >= 0 && static_cast<size_t>(node->parentIndex) < asset.LinearNodes.size()) {
			node->parent = asset.LinearNodes[static_cast<size_t>(node->parentIndex)].get();
		}
		else {
			node->parent = nullptr;
		}
	}

	return asset;
}

ModelLOD ModelUploader::uploadVertexList(Device& device, AssetManager& assets, std::unique_ptr<IVertexData> vertices, std::vector<uint32_t>& indices)
{
	ModelLOD asset{};

	asset.vertexBuffer = createVertexBuffers(device, vertices.get());
	asset.indexBuffer = createIndexBuffers(device, indices);

	Primitive newPrimitive{};
	newPrimitive.firstIndex = 0;
	newPrimitive.indexCount = indices.size();
	newPrimitive.materialIndex = 0;

	std::unique_ptr<Node> node = std::make_unique<Node>();
	node->primitives.reserve(1);
	node->primitives.push_back(std::move(newPrimitive));

	asset.LinearNodes.push_back(std::move(node));
	asset.nodes.push_back(asset.LinearNodes[0].get());

	asset.vertexCount = vertices->vertexCount();
	asset.indexCount = indices.size();

	asset.vertexStride = vertices->stride();

	return asset;
}

std::unique_ptr<Buffer> ModelUploader::createVertexBuffers(Device& device, const IVertexData* vertices)
{
	uint32_t vertexCount = vertices->vertexCount();

	if (vertexCount < 3) {
		throw std::exception("model uploader : Vertex count must be at least 3");
	}

	const uint32_t stride = vertices->layout().stride();
	const VkDeviceSize bufferSize = stride * vertexCount;

	Buffer stagingBuffer{
		device,
		stride,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices->rawData());

	std::unique_ptr<Buffer> vertexBuffer = std::make_unique<Buffer>(
		device,
		stride,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);

	return std::move(vertexBuffer);
}

std::unique_ptr<Buffer> ModelUploader::createIndexBuffers(Device& device, const std::vector<uint32_t>& indices)
{
	if (indices.empty()) {
		throw std::runtime_error("Index buffer cannot be empty");
	}

	const uint32_t indexCount = static_cast<uint32_t>(indices.size());
	const VkDeviceSize bufferSize = sizeof(uint32_t) * indexCount;

	Buffer stagingBuffer{
		device,
		sizeof(uint32_t),
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	std::unique_ptr<Buffer> indexBuffer = std::make_unique<Buffer>(
		device,
		sizeof(uint32_t),
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);

	return indexBuffer;
}

std::vector<TextureManager::TextureID> ModelUploader::uplaoadTextureList(Device& device, AssetManager& assets, const std::vector<ToBeDecodedTexture>& textures)
{

	std::vector<TextureManager::TextureID> outTextures;
	outTextures.reserve(textures.size());

	for (const auto& text : textures)
	{
		TextureBuilder builder(device);
		if (!text.textureName.empty())
		{
			outTextures.push_back(
				assets.textures().create(builder.fromFile(text.textureName)));
		}
		else if (!text.rawData.empty())
		{
			uint32_t mipLevels = static_cast<uint32_t>(floor(log2(std::max(text.width, text.height))) + 1.0);

			outTextures.push_back(
				assets.textures().create(
					builder.fromCharBuffer(
						text.rawData, text.width, text.height, 4, mipLevels)
				)
			);
		}
	}

	return outTextures;
}

std::vector<Material> ModelUploader::uploadMaterialsTextures(Device& device, AssetManager& assets, std::vector<DecodedMaterial> materials, const std::vector<TextureManager::TextureID>& loadedTextures)
{
	std::vector<Material> outMaterials{};

	for (auto mat : materials) {
		Material targetMat;

		//// albedo ////
		{
			if (!mat.albedoTexture.empty())
			{
				TextureBuilder builder(device);
				targetMat.albedoTexture = assets.textures().create(builder.fromFile(mat.albedoTexture));
			}
			else if (mat.baseColorTextureIndex != -1) {
				targetMat.albedoTexture = loadedTextures[mat.baseColorTextureIndex];
			}
			else {
				TextureBuilder builder(device);
				targetMat.albedoTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
			}
		}

		//// normal ////
		{
			if (!mat.normalTexture.empty())
			{
				TextureBuilder builder(device);
				targetMat.normalTexture = assets.textures().create(builder.fromFile(mat.normalTexture));
			}
			else if (mat.normalTextureIndex != -1) {
				targetMat.normalTexture = loadedTextures[mat.normalTextureIndex];
			}
			else {
				TextureBuilder builder(device);
				targetMat.normalTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
			}
		}

		//// metallic ////
		{
			if (!mat.metallicRoughnessTexture.empty())
			{
				TextureBuilder builder(device);
				targetMat.metallicRoughnessTexture = assets.textures().create(builder.fromFile(mat.metallicRoughnessTexture));
			}
			else if (mat.normalTextureIndex != -1) {
				targetMat.metallicRoughnessTexture = loadedTextures[mat.metallicRoughnessTextureIndex];
			}
			else {
				TextureBuilder builder(device);
				targetMat.metallicRoughnessTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
			}
		}

		//// occlusion ////
		{
			if (mat.occlusionTextureIndex != -1) {
				targetMat.occlusionTexture = loadedTextures[mat.occlusionTextureIndex];
			}
			else {
				TextureBuilder builder(device);
				targetMat.occlusionTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
			}
		}

		//// emissive ////
		{
			if (mat.emissiveTextureIndex != -1) {
				targetMat.emissiveTexture = loadedTextures[mat.emissiveTextureIndex];
			}
			else {
				TextureBuilder builder(device);
				targetMat.emissiveTexture = assets.textures().create(builder.fromFile("textures/whiteTexture.jpg"));
			}
		}

		targetMat.metallic = mat.metallic;
		targetMat.roughness = mat.roughness;
		targetMat.alphaCutoff = mat.alphaCutoff;
		targetMat.baseColorFactor = mat.baseColorFactor;
		targetMat.doubleSided = mat.doubleSided;

		outMaterials.push_back(targetMat);
	}

	return outMaterials;
}
