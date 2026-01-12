#pragma once

#include "BaseRenderSystem.h"

struct alignas(16) PushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	glm::mat4 normalMatrix{ 1.f };
};

class DescriptorSetLayout;

class MainMeshRenderSystem : public BaseRenderSystem {
public:
	
	MainMeshRenderSystem(
		Device& device, 
		AssetManager& assets, 
		IVertexLayout* vertexLayout, 
		RenderSystemConfig& config
	);

	/// render model: bind + draw
	void renderModel(
		VkCommandBuffer cmd,
		FrameContext& frameContext,
		const ModelAsset& model,
		uint32_t lodIndex, 
		glm::mat4 modelMat, glm::mat4 normalM
	) const;

protected:
	VkShaderStageFlagBits pushStage() const override;
	uint32_t pushSize() const override;

	void configurePipeline(
		PipelineConfigInfo& pipelineConfig
	) const {
	}; // no additionnal config needed


	// create model specific set layout
	std::vector<VkDescriptorSetLayout>
		createSetLayout(
			RenderSystemConfig& config
		);

private:
	// draw all nodes primitives
	void drawNode(
		VkCommandBuffer& commandBuffer,
		const ModelLOD& model, 
		Node* node, 
		uint16_t frameIndex,
		glm::mat4 modelMat, glm::mat4 normalM
	) const ;

	/// bind model specific
	void bindModelDescriptors(
		VkCommandBuffer cmd,
		const ModelLOD& model,
		uint32_t materialIndex,
		uint32_t frameIndex
	) const;

	/// Issue draw calls
	void drawModel(
		VkCommandBuffer cmd,
		const ModelAsset& model,
		FrameContext& frameContext,
		uint32_t lodIndex, uint32_t frameIndex,
		glm::mat4 modelMat, glm::mat4 normalM
	) const;

	void drawPrimitive(
		VkCommandBuffer& commandBuffer, 
		Primitive& primitive, 
		glm::mat4 modelMat, glm::mat4 normalM
	) const;

	uint32_t modelDescriptorSetIndex; 
	std::vector<DescriptorSetLayout> layouts;

};