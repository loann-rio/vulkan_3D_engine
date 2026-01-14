#include "BaseRenderSystem.h"


#include "../../model/Vertex/IVertexLayout.h"
#include "../../base/Device.h"
#include "../../base/Pipeline.h"
#include "../FrameContext.h"

#include <exception>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

BaseRenderSystem::BaseRenderSystem(Device& device, AssetManager& assets, IVertexLayout* layout, RenderSystemConfig& config) 
	: device{ device }, assets{ assets }, vertexLayout_{layout}
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayout 
		= createSetLayout(config);

	createPipelineLayout(config, descriptorSetLayout);
	createPipeline(config);
}

BaseRenderSystem::~BaseRenderSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void BaseRenderSystem::record(VkCommandBuffer cmd, FrameContext& frameContext) const
{
	bindPipeline(cmd);

	for (auto& obj : frameContext.listGameObjects)
	{
		if (obj->show && !obj->toBeRemoved)
		{
			if (!obj->modelAsset) continue;
			
			auto modelAsset = assets.models().get(obj->modelAsset);
			renderModel(cmd, frameContext, *modelAsset, 0, obj->getTransformMat(), obj->getNormalMat());
			
		}
	}
}

void BaseRenderSystem::configVertexBindingDescription(PipelineConfigInfo& pipelineConfig)
{
	assert(vertexLayout().stride() != 0 && "pipeline creation: stride must not be null");

	std::vector<VkVertexInputBindingDescription> bindingDescription(1);
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = vertexLayout().stride();
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	pipelineConfig.bindingDescription = bindingDescription;
}

void BaseRenderSystem::configVertexAttributeDescription(PipelineConfigInfo& pipelineConfig)
{
	std::vector<VkVertexInputAttributeDescription> attributeDescription;

	for (uint32_t i = 0; i < vertexLayout().attributeCount(); i++) {
		attributeDescription.push_back(
			{
				i, 0,
				vertexLayout().attributes()[i].format,
				vertexLayout().attributes()[i].offset
			}
		);
	}

	if (attributeDescription.empty()) {
		throw std::exception("pipeline creation :vertex layout attribute connot be empty if defined");
	}

	pipelineConfig.attributeDescription = attributeDescription;
}

void BaseRenderSystem::bindPipeline(VkCommandBuffer cmd) const
{
	pipeline->bind(cmd);
}

const IVertexLayout& BaseRenderSystem::vertexLayout() const
{
	return *vertexLayout_;
}

void BaseRenderSystem::createPipelineLayout(RenderSystemConfig& config, std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{
	VkPushConstantRange pushConstantRange{
		pushStage(),
		0, /*offset*/
		pushSize()
	};

	VkDescriptorBindingFlags descriptorBindingFlags[] = {
		0,											  // For non-dynamic descriptor sets
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT // For dynamic descriptor sets
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.data();

	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	// create layout
	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("fail to create pipeline layout");
	}
}

void BaseRenderSystem::createPipeline(RenderSystemConfig& config)
{
	// check if pipeline layout is created
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	// get default pipeline configuration
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	// config render system specific
	configurePipeline(pipelineConfig);

	// config vertex description
	if (vertexLayout_) {
		configVertexBindingDescription(pipelineConfig);
		configVertexAttributeDescription(pipelineConfig);
	}

	// alpha
	if (config.alphaBlend) Pipeline::enableAlphaBlending(pipelineConfig);

	// render pass and pipeline layout
	pipelineConfig.renderPass = config.renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;

	// create the pipeline
	pipeline = std::make_unique<Pipeline>(
		device,
		config.vertexShaderPath,
		config.fragmentShaderPath,
		pipelineConfig
	);
}

