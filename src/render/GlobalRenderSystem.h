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


struct RenderSystemBuilder {
	std::string vertFilepath;
	std::string fragFilepath = "";
	std::vector<VkDescriptorSetLayout> globalSetLayout;
	VkRenderPass renderPass;
	bool hasMultipleInstance = false;
	ModelSubType subModelType = ModelSubType::NONE;
	bool isSkyBox = false;
	bool isFullscreenRender = false;
	VkShaderStageFlagBits pushStage;
};

struct RenderSystemConfig
{
	VkRenderPass renderPass{}; // need check at build

	std::string vertexShader; // need check at build
	std::string fragmentShader; // need check at build

	ModelType modelType = ModelType::UNDEFINED_MODEL; // need check at build
	ModelSubType modelSubType = ModelSubType::NONE; // need check at build

	std::vector<VkDescriptorSetLayout> globalLayouts{}; // need check at build
	std::vector<DescriptorSetObject> descriptorBindings; // define + need check at build

	std::vector<VkVertexInputBindingDescription> bindingDescriptions; // define + need check at build
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions; // define + need check at build

	uint16_t modelDescriptorSetIndex; 

	VkShaderStageFlags pushStage = 0;  // define + need check at build

	bool shadow = false;
	bool skybox = false;
	bool fullscreen = false;
};

class GlobalRenderSystem
{

public:

	GlobalRenderSystem(
		Device& device,
		AssetManager& assets, 
		RenderSystemConfig config
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete; 
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete; 

	void renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, const std::array<FrustumPlane, 6>& frustrumPlanes = {});
	void renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex);
	void renderFullScreen(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets, glm::mat4 view, glm::mat4 proj);

private:

	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass, 
		const std::string& vertFilepath, const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription);

	void renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj, const std::array<FrustumPlane, 6>& frustrumPlanes);
	void renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex, uint16_t frameIndex, const std::array<FrustumPlane, 6>& planes);

	void bind(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets); 

	void bindModel(VkCommandBuffer& commandBuffer, ModelAsset* model);
	void bindTextures(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, uint16_t frameIndex);
	void drawModel(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, glm::mat4 modelMat, glm::mat4 normalM);

	Device& device;
	AssetManager& assets;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;

	ModelType modelType = ModelType::UNDEFINED_MODEL;
	ModelSubType modelSubType = ModelSubType::NONE;
	const bool isShadow = false;
	const bool isSkyBox = false;
	const bool isFullscreenRender = false;

	bool customPushStage = false;
	VkShaderStageFlagBits pushStage;

	uint16_t modelDescriptorSetIndex; // start after global, shadow add additional descriptor set
		
};