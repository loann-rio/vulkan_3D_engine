#pragma once

#include "../base/Pipeline.h"
#include "../base/device.h"

#include "../base/Frame_info.h"
#include "../base/descriptors.h"
#include "../objects/GameObject.h"
#include "../render/Camera.h"

#include "../model/ModelAsset.h"

#include "../assetManager/AssetManager.h"

#include <memory>
#include <vector>

struct alignas(16) SimplePushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	glm::mat4 normalMatrix{ 1.f };
};

struct alignas(16) MaterialPushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	uint32_t materialIndex{ 0 };
	uint32_t nodeIndex{ 0 };
	uint32_t jointOffset{ 0 };
	uint32_t jointCount{ 0 };
};

struct alignas(16) DepthPushConstantData {
	glm::mat4 modelMatrix{ 1.f };
	uint32_t indexDepthCamera{ 0 };
	uint32_t nodeIndex{ 0 };
	uint32_t jointOffset{ 0 };
	uint32_t jointCount{ 0 };
};

struct RenderSystemConfig
{
	VkRenderPass renderPass = VK_NULL_HANDLE;

	std::string vertexShader;
	std::string fragmentShader; 

	ModelType modelType = ModelType::UNDEFINED_MODEL;
	ModelSubType modelSubType = ModelSubType::NONE; 

	std::vector<VkDescriptorSetLayout> globalLayouts{}; 
	std::vector<DescriptorSetObject> descriptorBindings; 

	std::vector<VkVertexInputBindingDescription> bindingDescriptions; 
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions; 

	uint16_t modelDescriptorSetIndex; 

	VkShaderStageFlags pushStage = 0; 

	bool shadow = false;
	bool skybox = false;
	bool fullscreen = false;
	bool instanced = true;
	bool materialBuffer = false;
	bool poseBuffer = false;
};

class RenderSystem
{
public:
	RenderSystem(
		Device& device,
		AssetManager& assets, 
		RenderSystemConfig config
	);

	~RenderSystem();

	RenderSystem(const RenderSystem&) = delete; 
	RenderSystem& operator=(const RenderSystem&) = delete; 

	void renderGameObjects(
		VkCommandBuffer& commandBuffer, 
		FrameInfo& frameInfo, 
		std::vector<VkDescriptorSet> globalDescriptorSets, 
		const std::array<FrustumPlane, 6>& frustrumPlanes = {}
	);

	void renderGameObjectsDepth(
		VkCommandBuffer& commandBuffer, 
		FrameInfo& frameInfo, 
		std::vector<VkDescriptorSet> globalDescriptorSets, 
		int lightIndex, 
		uint16_t frameIndex
	);

	void renderFullScreen(
		VkCommandBuffer& commandBuffer, 
		std::vector<VkDescriptorSet> globalDescriptorSets, 
		glm::mat4 view, 
		glm::mat4 proj
	);

private:

	void createPipelineLayout(
		std::vector<VkDescriptorSetLayout> descriptorSetLayout
	);

	void createPipeline(
		VkRenderPass renderPass, 
		const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription
	);

	void bindGlobalSets(
		VkCommandBuffer& commandBuffer, 
		std::vector<VkDescriptorSet> globalDescriptorSets
	); 

	void bindModel(
		VkCommandBuffer& commandBuffer, 
		ModelAsset* model,
		GameObjectModel& obj,
		uint16_t frameIndex
	);

	void bindTextures(
		VkCommandBuffer& commandBuffer, 
		ModelAsset* model, 
		Primitive& primitive, 
		uint16_t frameIndex
	);

	void bindMaterialBuffer(
		VkCommandBuffer& commandBuffer,
		ModelAsset* model,
		uint16_t frameIndex
	);

	void drawModel(
		VkCommandBuffer& commandBuffer, 
		ModelAsset* model, 
		Primitive& primitive, 
		glm::mat4 modelMat, 
		glm::mat4 normalM,
		uint32_t instanceCount
	);

	void drawDepth(
		VkCommandBuffer& commandBuffer, 
		ModelAsset* model, 
		Primitive& primitive, 
		glm::mat4 modelMat, 
		uint32_t cameraIndex,
		uint32_t instanceCount
	);

	bool bindPose(
		VkCommandBuffer& commandBuffer,
		GameObjectModel& obj,
		uint16_t frameIndex
	);

	Device& device;
	AssetManager& assets;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;

	ModelType modelType = ModelType::UNDEFINED_MODEL;
	ModelSubType modelSubType = ModelSubType::NONE;
	const bool isShadow = false;
	const bool isSkyBox = false;
	const bool isFullscreenRender = false;
	const bool hasInstanceBinding = false;
	const bool useMaterialBuffer = false;
	const bool usePoseBuffer = false;

	std::unique_ptr<Buffer> dummyInstanceBuffer = nullptr;

	bool customPushStage = false;
	VkShaderStageFlagBits pushStage;

	uint16_t modelDescriptorSetIndex; // start after global, shadow add additional descriptor set
};