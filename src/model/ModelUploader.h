#pragma once

#include "Decoder/IModelDecoder.h"
#include "ModelAsset.h"

#include "../assetManager/AssetManager.h"

#include <memory>

class ModelUploader {

public:
	ModelUploader() = default;
	~ModelUploader() = default;

	static ModelLOD uploadDecodedModel(Device& device, AssetManager& assets, DecodedModel& obj);

private:
	static std::unique_ptr<Buffer> createVertexBuffers(Device& device, IVertexData* vertices);
	static std::unique_ptr<Buffer> createIndexBuffers(Device& device, const std::vector<uint32_t>& indices);

	static std::vector<Material> uploadMaterialsTextures(Device& device, AssetManager& assets, std::vector<DecodedMaterial> materials);

};