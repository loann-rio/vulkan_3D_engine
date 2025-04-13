#pragma once

#include "Pipeline.h"
#include "device.h"
#include "GameObject.h"
#include "Camera.h"
#include "Frame_info.h"
#include "descriptors.h"

#include <memory>
#include <vector>


class GlobalRenderSystem
{

public:

	template <class T> static std::shared_ptr<GlobalRenderSystem> create(Device& device, VkRenderPass renderPass,
		std::vector<VkDescriptorSetLayout> globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath = "");

	GlobalRenderSystem(Device& device, VkRenderPass renderPass,  
		std::vector<VkDescriptorSetLayout> globalSetLayout, std::vector<DescriptorObject> bindings,
		const std::string& vertFilepath, const std::string& fragFilepath,
		ModelType modelType,
		std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription,
		bool isShadow = false 
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete; 
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete; 

	void renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets); 
	void renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex);
	
	void setType(ModelType type) { modelType = type; }

private:

	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass, 
		const std::string& vertFilepath, const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription);

	void renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObjectModel* obj);
	void renderModelDepth(VkCommandBuffer& commandBuffer, GameObjectModel* obj, int lightIndex);
	
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
inline std::shared_ptr<GlobalRenderSystem> GlobalRenderSystem::create(Device& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>  globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath)
{
	std::vector<DescriptorObject> bindings;
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions(false); 
	ModelType modelType = static_cast<ModelType>(T::getModelType()); 

	bool isShadow = (fragFilepath == "");
	if (isShadow) {
		bindings = std::vector<DescriptorObject>();
		attributeDescription = T::Vertex::getAttributeDescriptionsShadow(false);
	}
	else {
		bindings = T::getDescriptorType();  
		attributeDescription = T::Vertex::getAttributeDescriptions(false);
	}

	return std::make_shared<GlobalRenderSystem>(device, renderPass,
		globalSetLayout, bindings,
		vertFilepath, fragFilepath,
		modelType,  
		bindingDescription, attributeDescription, isShadow);
} 

