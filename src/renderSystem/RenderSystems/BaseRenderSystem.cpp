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

BaseRenderSystem::BaseRenderSystem(
	Device& device_,
	AssetManager& assets_,
	const IVertexLayout& vertexLayout_,
	const RenderSystemCreateInfo& createInfo_
)
	: device(device_)
	, assets(assets_)
	, vertexLayout(vertexLayout_)
	, createInfo(createInfo_)
{ }

BaseRenderSystem::~BaseRenderSystem()
{
	if (pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}
}

void BaseRenderSystem::record(
	VkCommandBuffer cmd,
	FrameContext& frameContext,
	const std::vector<RenderItem>& items
) const
{
	bindPipeline(cmd);

	for (const RenderItem& item : items) {
		if (!item.model) continue;
		renderModel(cmd, frameContext, item);
	}
}

void BaseRenderSystem::bindPipeline(VkCommandBuffer cmd) const
{
	pipeline->bind(cmd);
}

void BaseRenderSystem::createPipelineLayout(DescriptorSetLayout* globalSetLayout)
{
	std::vector<VkDescriptorSetLayout> vkLayouts;
	vkLayouts.reserve(
		(globalSetLayout ? 1 : 0) + 
		descriptorLayouts.size()
	);

	// global layouts
	if (globalSetLayout != nullptr) {
		vkLayouts.push_back(globalSetLayout->getDescriptorSetLayout());
	}

	// local layouts
	for (const auto& layout : descriptorLayouts) {
		vkLayouts.push_back(layout->getDescriptorSetLayout());
	}

	// push constants
	PushConstantInfo pc = pushConstants();
	assert(pc.size <= device.properties.limits.maxPushConstantsSize);

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = pc.stages;
	pushRange.offset = 0;
	pushRange.size = pc.size;

	// create info
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.setLayoutCount = static_cast<uint32_t>(vkLayouts.size());
	info.pSetLayouts = vkLayouts.data();
	info.pushConstantRangeCount = pc.size > 0 ? 1u : 0u;
	info.pPushConstantRanges = pc.size > 0 ? &pushRange : nullptr;

	if (vkCreatePipelineLayout(device.device(), &info, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void BaseRenderSystem::createPipeline(const RenderSystemCreateInfo& ci)
{
	assert(pipelineLayout != VK_NULL_HANDLE);

	PipelineConfigInfo config{};
	Pipeline::defaultPipelineConfigInfo(config);

	/// vertex input
	configureVertexInput(config);

	/// raster state
	config.rasterizationInfo.cullMode = ci.cullMode;
	config.rasterizationInfo.frontFace = ci.frontFace;

	/// depth
	config.depthStencilInfo.depthTestEnable = ci.depthTest;
	config.depthStencilInfo.depthWriteEnable = ci.depthWrite;
	config.depthStencilInfo.depthCompareOp = ci.depthCompare;

	/// blending
	if (ci.alphaBlend) {
		Pipeline::enableAlphaBlending(config);
	}

	config.renderPass = ci.renderPass;
	config.pipelineLayout = pipelineLayout;

	configurePipeline(config);

	pipeline = std::make_unique<Pipeline>(
		device,
		ci.vertexShaderPath,
		ci.fragmentShaderPath,
		config
	);
}


void BaseRenderSystem::configureVertexInput(
	PipelineConfigInfo& pipelineConfig
) const
{
	assert(vertexLayout.stride() > 0);

	pipelineConfig.bindingDescription = {
		{
			0,
			vertexLayout.stride(),
			VK_VERTEX_INPUT_RATE_VERTEX
		}
	};

	pipelineConfig.attributeDescription.clear();
	pipelineConfig.attributeDescription.reserve(vertexLayout.attributeCount());

	for (uint32_t i = 0; i < vertexLayout.attributeCount(); ++i) {
		const auto& attr = vertexLayout.attributes()[i];
		pipelineConfig.attributeDescription.push_back({
			i,
			0,
			attr.format,
			attr.offset
			});
	}

	if (pipelineConfig.attributeDescription.empty()) {
		throw std::runtime_error("Vertex layout has no attributes");
	}
}