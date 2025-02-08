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
		std::vector<VkDescriptorSetLayout> globalSetLayout, const std::string& vertFilepath, const std::string& fragFilepath);

	template <class T> static std::shared_ptr<GlobalRenderSystem> create(Device& device, VkRenderPass renderPass,
		std::vector<VkDescriptorSetLayout> globalSetLayout, const std::string& vertFilepath);

	GlobalRenderSystem(Device& device, VkRenderPass renderPass, 
		std::vector<VkDescriptorSetLayout> globalSetLayout, std::vector<VkDescriptorType> bindings, 
		const std::string& vertFilepath, const std::string& fragFilepath,
		ModelType modelType,
		std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription,
		bool isShadow = false
	);

	~GlobalRenderSystem();

	GlobalRenderSystem(const GlobalRenderSystem&) = delete;
	GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete;

	//GlobalRenderSystem(GlobalRenderSystem&&) noexcept = default; 
	//GlobalRenderSystem& operator=(GlobalRenderSystem&&) noexcept = default; 

	void renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo);
	void setType(ModelType type) { modelType = type; }

private:

	void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass, 
		const std::string& vertFilepath, const std::string& fragFilepath, 
		std::vector<VkVertexInputBindingDescription> bindingDescription, 
		std::vector<VkVertexInputAttributeDescription> attributeDescription);

	void renderModel(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, GameObject& obj);

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
	std::vector<VkDescriptorType> bindings = T::getDescriptorType();
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions();
	std::vector<VkVertexInputAttributeDescription> attributeDescription = T::Vertex::getAttributeDescriptions();
	ModelType modelType = static_cast<ModelType>(T::getModelType());

	auto renderSystem = new GlobalRenderSystem(
		device, renderPass,
		globalSetLayout, bindings,
		vertFilepath, fragFilepath,
		modelType,
		bindingDescription, attributeDescription, false
	);

	std::cout << "helllo \n";

	return std::shared_ptr<GlobalRenderSystem>(renderSystem);
} 

template<class T>
inline std::shared_ptr<GlobalRenderSystem> GlobalRenderSystem::create(Device& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> globalSetLayout, const std::string& vertFilepath)
{
	std::vector<VkVertexInputBindingDescription> bindingDescription = T::Vertex::getBindingDescriptions();
	std::vector<VkVertexInputAttributeDescription> attributeDescription = T::Vertex::getAttributeDescriptionsShadow(); 
	ModelType modelType = static_cast<ModelType>(T::getModelType());

	auto renderSystem = new GlobalRenderSystem(
		device, renderPass,
		globalSetLayout, std::vector<VkDescriptorType>(),
		vertFilepath, "",
		modelType,
		bindingDescription, attributeDescription, true
	);

	return std::shared_ptr<GlobalRenderSystem>(renderSystem);
}
