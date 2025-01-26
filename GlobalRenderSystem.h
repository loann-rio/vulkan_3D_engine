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

	template <class T> static GlobalRenderSystem create(Device& device, VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath);

	template <class T> static GlobalRenderSystem createDepth(Device& device, VkRenderPass renderPass, 
		VkDescriptorSetLayout globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath); 

	GlobalRenderSystem(Device& device, VkRenderPass renderPass, 
		VkDescriptorSetLayout globalSetLayout, std::vector<VkDescriptorType> bindings, 
		const std::string& vertFilepath, const std::string& fragFilepath,
		std::string modelType,
		std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription,
		bool isShadow = false
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete;
	
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete;

	void renderGameObjects(FrameInfo& frameInfo);

private:

	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass, 
		const std::string& vertFilepath, const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription);

	void renderObjModel(FrameInfo& frameInfo, GameObject& obj);

	Device& device;

	std::unique_ptr<Pipeline> objPipeline;
	VkPipelineLayout objPipelineLayout;

	std::unique_ptr<Pipeline> GlTFPipeline;
	VkPipelineLayout GlTFPipelineLayout;

	const std::string modelType;
	const bool isShadow = false;

	
};

template<class T>
inline GlobalRenderSystem GlobalRenderSystem::create(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath)
{
	std::vector<VkDescriptorType> bindings = T::getDescriptorType();
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions();
	std::vector<VkVertexInputAttributeDescription> attributeDescription = T::Vertex::getAttributeDescriptions();
	std::string modelType = T::getType();

	return GlobalRenderSystem(
		device, renderPass, 
		globalSetLayout, bindings, 
		vertFilepath, fragFilepath, 
		modelType,
		bindingDescription, attributeDescription
	);
}

template<class T>
inline GlobalRenderSystem GlobalRenderSystem::createDepth(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath)
{
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptionsShadow();
	std::vector<VkVertexInputAttributeDescription> attributeDescription = T::Vertex::getAttributeDescriptionsShadow(); 
	std::string modelType = T::getType(); 

	return GlobalRenderSystem( 
		device, renderPass,
		globalSetLayout, {},
		vertFilepath, fragFilepath,
		modelType, 
		bindingDescription, attributeDescription , true
	);
}
