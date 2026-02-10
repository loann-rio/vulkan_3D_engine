#pragma once

#include "BaseRenderSystem.h"

#include <glm/glm.hpp>
#include <vector>

class ModelLOD;
class Node;
class Primitive;

class FullScreenRenderSystem : public BaseRenderSystem {
	struct alignas(16) PushConstantData {
		
	};

public:

	FullScreenRenderSystem(
		Device& device,
		AssetManager& assets,
		const IVertexLayout& vertexLayout,
		const RenderSystemCreateInfo& createInfo
	);

protected:
	PushConstantInfo pushConstants() const override;

	void createDescriptorSetLayouts(
		std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
	) override;

	void renderModel(
		VkCommandBuffer cmd,
		FrameContext& frameContext,
		const RenderItem& item
	) const override;

private:

	void bindMaterial(
		VkCommandBuffer cmd,
		const ModelLOD& lod,
		uint32_t materialIndex,
		uint32_t frameIndex
	) const;

	std::vector<DescriptorSetLayout> layouts;

};