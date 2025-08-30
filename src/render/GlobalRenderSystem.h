#pragma once

#include "../base/Pipeline.h"
#include "../base/device.h"

#include "../base/Frame_info.h"
#include "../base/descriptors.h"
#include "../objects/GameObject.h"
#include "../render/Camera.h"
#include <memory>
#include <vector>


struct RenderSystemBuilder {
	std::string vertFilepath;
	std::string fragFilepath = "";
	std::vector<VkDescriptorSetLayout> globalSetLayout;
	VkRenderPass renderPass;
	bool hasMultipleInstance = false;
	ModelSubType subModelType = NONE;
};

class GlobalRenderSystem
{

public:

	template <class T> static std::shared_ptr<GlobalRenderSystem> create(Device& device, VkRenderPass renderPass,
		std::vector<VkDescriptorSetLayout> globalSetLayout, const std::string& vertFilepath, bool hasMultipleInstance = false, const std::string& fragFilepath = "");

	template <class T> static std::shared_ptr<GlobalRenderSystem> create(Device& device, RenderSystemBuilder builder);

	GlobalRenderSystem(Device& device, VkRenderPass renderPass,  
		std::vector<VkDescriptorSetLayout> globalSetLayout, std::vector<DescriptorSetObject> bindings, 
		const std::string& vertFilepath, const std::string& fragFilepath,
		ModelType modelType, ModelSubType subModelType, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription,
		bool isShadow = false
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete; 
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete; 

	void renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets); 
	void renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex);
	
	void setType(ModelType type) { modelType = type; }

private:

	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass, 
		const std::string& vertFilepath, const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription);

	void renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj);
	void renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex, uint16_t frameIndex);
	
	void bind(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets); 

	Device& device;

	std::unique_ptr<Pipeline> objPipeline;
	VkPipelineLayout objPipelineLayout;

	std::unique_ptr<Pipeline> GlTFPipeline;
	VkPipelineLayout GlTFPipelineLayout;

	ModelType modelType = UNDEFINED_MODEL;
	const bool isShadow = false;
		
};

template<class T>
inline std::shared_ptr<GlobalRenderSystem> GlobalRenderSystem::create(Device& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>  globalSetLayout, const std::string& vertFilepath, bool hasMultipleInstance, const std::string& fragFilepath)
{
	std::vector<DescriptorSetObject> bindings;
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions(hasMultipleInstance);
	ModelType modelType = static_cast<ModelType>(T::getModelType());  

	bool isShadow = (fragFilepath == "");
	if (isShadow) {
		bindings = T::getDescriptorType(); //std::vector<DescriptorSetObject>();
		attributeDescription = T::Vertex::getAttributeDescriptionsShadow(hasMultipleInstance);
	}
	else {
		bindings = T::getDescriptorType(); 
 		attributeDescription = T::Vertex::getAttributeDescriptions(hasMultipleInstance);
	}

	return std::make_shared<GlobalRenderSystem>(device, renderPass,
		globalSetLayout, bindings,
		vertFilepath, fragFilepath,
		modelType, ModelSubType::NONE,
		bindingDescription, attributeDescription, isShadow);
}

template<class T>
inline std::shared_ptr<GlobalRenderSystem> GlobalRenderSystem::create(Device& device, RenderSystemBuilder builder)
{
	std::vector<DescriptorSetObject> bindings;
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions(builder.hasMultipleInstance);
	
	ModelType modelType = static_cast<ModelType>(T::getModelType());

	bool isShadow = (builder.fragFilepath == "");
	if (isShadow) {
		bindings = T::getDescriptorType();
		attributeDescription = T::Vertex::getAttributeDescriptionsShadow(builder.hasMultipleInstance);
	}
	else {
		bindings = T::getDescriptorType();
		attributeDescription = T::Vertex::getAttributeDescriptions(builder.hasMultipleInstance);
	}

	return std::make_shared<GlobalRenderSystem>(
		device, 
		builder.renderPass,
		builder.globalSetLayout, 
		bindings,
		builder.vertFilepath, builder.fragFilepath,
		modelType, builder.subModelType, 
		bindingDescription, attributeDescription, 
		isShadow);
}

