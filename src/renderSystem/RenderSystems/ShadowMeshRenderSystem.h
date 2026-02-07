
#pragma once

#include "BaseRenderSystem.h"

#include <glm/glm.hpp>
#include <vector>

class ModelLOD;
class Node;
class Primitive;

struct alignas(16) PushConstantData {
	glm::mat4 modelMatrix{ 1.f };
};

class ShadowMeshRenderSystem : public BaseRenderSystem {
public:

	ShadowMeshRenderSystem(
		Device& device,
		AssetManager& assets,
		const IVertexLayout& vertexLayout,
		const RenderSystemCreateInfo& createInfo
	);

	bool accepts(const GameObjectModel& object) const;

protected:
	PushConstantInfo pushConstants() const override;

	void createDescriptorSetLayouts(
		std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
	) override {};

	void renderModel(
		VkCommandBuffer cmd,
		FrameContext& frameContext,
		const RenderItem& item
	) const override;

private:
	void drawLOD(
		VkCommandBuffer cmd,
		FrameContext& frameContext,
		const ModelLOD& lod,
		const glm::mat4& modelMat
	) const;

	void drawNode(
		VkCommandBuffer cmd,
		FrameContext& frameContext,
		const ModelLOD& lod,
		const Node* node,
		const glm::mat4& modelMat
	) const;

	void drawPrimitive(
		VkCommandBuffer cmd,
		const Primitive& primitive,
		const glm::mat4& modelMat
	) const;

	std::vector<DescriptorSetLayout> layouts;

};