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

class GlobalRenderSystem
{

public:

	// external builder to allow the use of template, take a RenderSystemBuilder as arg
	template <class T> static std::shared_ptr<GlobalRenderSystem> create(Device& device, AssetManager& assets, RenderSystemBuilder builder);

	GlobalRenderSystem(Device& device, AssetManager& assets,
		VkRenderPass renderPass,  
		std::vector<VkDescriptorSetLayout> globalSetLayout, std::vector<DescriptorSetObject> bindings, 
		const std::string& vertFilepath, const std::string& fragFilepath,
		ModelType modelType, ModelSubType subModelType, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription,
		VkShaderStageFlagBits pushStage, bool isShadow = false, bool isSkyBox = false, bool isFullscreenrender = false
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete; 
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete; 

	void renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, const std::array<FrustumPlane, 6>& frustrumPlanes = {});
	void renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex);
	void renderFullScreen(VkCommandBuffer& commandBuffer, VkDescriptorSet& globalDescriptorSets, glm::mat4 view, glm::mat4 proj);

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

/// <summary>
/// external builder to allow the use of template, get the attribute description;, model type and descriptor type from template model type
/// if frag shader not included, the render system will only render depth
/// </summary>
/// <typeparam name="T"> model type class ( to be changed to have a single model class ) </typeparam>
/// <param name="device"> device </param>
/// <param name="builder"> RenderSystemBuilder </param>
/// <returns> return an instance of render system </returns>
template<class T>
inline std::shared_ptr<GlobalRenderSystem> GlobalRenderSystem::create(Device& device, AssetManager& assets, RenderSystemBuilder builder)
{
	std::vector<DescriptorSetObject> bindings;
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	std::vector<VkVertexInputBindingDescription> bindingDescription; 

	ModelType modelType = static_cast<ModelType>(T::getModelType());

	bool isFullscreen = builder.isFullscreenRender;
	bool isShadow = (builder.fragFilepath == "");

	// Only populate vertex binding/attribute descriptions if the pipeline needs vertex input
	if (!isFullscreen) {
		bindingDescription = T::Vertex::getBindingDescriptions(builder.hasMultipleInstance);

		if (isShadow) {
			bindings = T::getDescriptorType();
			attributeDescription = T::Vertex::getAttributeDescriptionsShadow(builder.hasMultipleInstance);
		}
		else {
			bindings = T::getDescriptorType();
			attributeDescription = T::Vertex::getAttributeDescriptions(builder.hasMultipleInstance);
		}
	}
	else {
		// fullscreen: still may need descriptor bindings
		bindings = T::getDescriptorType();
		// leave bindingDescription and attributeDescription empty
	}

	return std::make_shared<GlobalRenderSystem>(
		device, assets,
		builder.renderPass,
		builder.globalSetLayout,
		bindings,
		builder.vertFilepath, builder.fragFilepath,
		modelType, builder.subModelType,
		bindingDescription, attributeDescription, builder.pushStage,
		isShadow, builder.isSkyBox, builder.isFullscreenRender);
}

