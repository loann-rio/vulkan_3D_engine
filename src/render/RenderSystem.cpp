#include "RenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/detail/qualifier.hpp>
#include <glm/fwd.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <cstddef>


namespace {

	struct NodeDrawInfo { uint32_t nodeIndex = 0, jointOffset = 0, jointCount = 0; };

	NodeDrawInfo nodeDrawInfo(const ModelAsset* model, const Primitive& primitive) {
		NodeDrawInfo info{};
		if (primitive.nodeIndex != UINT32_MAX) info.nodeIndex = primitive.nodeIndex;
		if (primitive.skinIndex != UINT32_MAX && primitive.skinIndex < model->skeleton.skins.size()) {
			const Skin& skin = model->skeleton.skins[primitive.skinIndex];
			info.jointOffset = skin.jointOffset;
			info.jointCount = static_cast<uint32_t>(skin.joints.size());
		}
		return info;
	}
}

RenderSystem::RenderSystem(Device& device, AssetManager& assets, RenderSystemConfig config):
	device{ device },
	assets{ assets },
	modelType{ config.modelType },
	modelSubType{ config.modelSubType },
	isShadow{ config.shadow },
	isSkyBox{ config.skybox },
	isFullscreenRender{ config.fullscreen },
	hasInstanceBinding{ config.instanced && !config.fullscreen },
	useMaterialBuffer{ config.materialBuffer && !config.shadow && !config.fullscreen },
	modelDescriptorSetIndex{ config.modelDescriptorSetIndex },
	customPushStage{ config.pushStage != 0 },
	usePoseBuffer{ config.poseBuffer },
	pushStage{ static_cast<VkShaderStageFlagBits>(config.pushStage) }
{
	// Build descriptor set layouts for model descriptorBindings and append to the global layouts
	std::vector<std::unique_ptr<DescriptorSetLayout>> layouts;
	// copy global layouts so we can append model layouts before creating pipeline layout
	std::vector<VkDescriptorSetLayout> layoutsToCreate = config.globalLayouts;

	for (size_t j = 0; j < config.descriptorBindings.size(); ++j) {
		auto builder = DescriptorSetLayout::Builder(device);
		for (int i = 0; i < config.descriptorBindings[j].descriptorSet.size(); ++i) 
		{
			const auto& desc = config.descriptorBindings[j].descriptorSet[i];
			builder.addBinding(i, desc.descriptorType, desc.stage, desc.count);
		}

		auto newLayout = builder.build();

		if (!newLayout) {
			std::cerr << "Failed to build descriptor set layout at index " << j << "\n";
			continue;
		}
		layoutsToCreate.push_back(newLayout->getDescriptorSetLayout());
		layouts.push_back(std::move(newLayout));
	}

	// create pipelinelayout with the global and model descriptor set layout
	createPipelineLayout(
		layoutsToCreate
	);

	// create pipeline
	createPipeline(
		config.renderPass, 
		config.vertexShader, 
		config.fragmentShader, 
		config.bindingDescriptions,
		config.attributeDescriptions
	);

	if (hasInstanceBinding) {
		ModelInstance identity{ glm::vec4(0.f), glm::vec4(0.f), glm::vec4(1.f) };

		dummyInstanceBuffer = std::make_unique<Buffer>(
			device,
			sizeof(ModelInstance),
			1,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		dummyInstanceBuffer->map();
		dummyInstanceBuffer->writeToBuffer(&identity);
		dummyInstanceBuffer->unmap();
	}
}

/// <summary>
/// destroy pipelinelayout at removal of object
/// </summary>
RenderSystem::~RenderSystem()
{
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;

	if (isShadow) {
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.size = sizeof(DepthPushConstantData);
	}
	else if (useMaterialBuffer) {
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.size = sizeof(MaterialPushConstantData);
	}
	else if (isFullscreenRender) {
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.size = sizeof(CubeMapPushConstantData);
	}
	else {
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.size = sizeof(SimplePushConstantData);
	}

	if (customPushStage) pushConstantRange.stageFlags = pushStage;

	VkDescriptorBindingFlags descriptorBindingFlags[] = {
	0,  // For non-dynamic descriptor sets
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

void RenderSystem::createPipeline(VkRenderPass renderPass, const std::string& vertFilepath, const std::string& fragFilepath, 
	std::vector<VkVertexInputBindingDescription> bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescription)
{
	// check if pipeline layout is created
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};

	// get default pipeline configuration
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);

	if (!bindingDescription.empty()) {
		pipelineConfig.bindingDescription = bindingDescription;
	}
	else {
		pipelineConfig.bindingDescription.clear();
	}

	if (!attributeDescription.empty()) {
		pipelineConfig.attributeDescription = attributeDescription;
	}
	else {
		pipelineConfig.attributeDescription.clear();
	}

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	
	// enable depth bias if no frag shader (depth only)
	if (fragFilepath.empty()) {
		pipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
		pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 4.0f;
		pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 1.5f;
	}

	if (isSkyBox) {
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	}

	if (isFullscreenRender) {
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	}

	// create the pipeline
	pipeline = std::make_unique<Pipeline>(
		device,
		vertFilepath,
		fragFilepath,
		pipelineConfig
	);
}

void RenderSystem::bindGlobalSets(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets)
{
	pipeline->bind(commandBuffer);

	for (uint16_t setIndex = 0; setIndex < globalDescriptorSets.size(); setIndex++)
	{
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			setIndex, 1,
			&globalDescriptorSets[setIndex],
			0, nullptr 
		);
	}
}

void RenderSystem::bindModel(VkCommandBuffer& commandBuffer, ModelAsset* model, GameObjectModel& obj, uint16_t frameIndex)
{

	VkBuffer vertexBuffer = model->lods[0].vertexBuffer->getBuffer();

	if (hasInstanceBinding) {
		Buffer* instanceBuffer = obj.getFrameInstancesBuffer(frameIndex);
		if (!instanceBuffer) instanceBuffer = dummyInstanceBuffer.get();

		VkBuffer buffers[] = { vertexBuffer, instanceBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0, 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
	}
	else {
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	}

	vkCmdBindIndexBuffer(commandBuffer, model->lods[0].indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void RenderSystem::bindTextures(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, uint16_t frameIndex) const
{
	vkCmdBindDescriptorSets(commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		modelDescriptorSetIndex, 1,
		&model->lods[0].materials[primitive.materialIndex].descriptorSet[frameIndex],
		0,
		nullptr);
}

void RenderSystem::bindMaterialBuffer(VkCommandBuffer& commandBuffer, ModelAsset* model, uint16_t frameIndex) const
{
	vkCmdBindDescriptorSets(commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		modelDescriptorSetIndex, 1,
		&model->lods[0].descriptorSet[frameIndex],
		0,
		nullptr);
}

void RenderSystem::drawModel(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, uint32_t modelMatrixIndex, uint32_t instanceCount = 1) const
{
	if (useMaterialBuffer) {
		const NodeDrawInfo node = nodeDrawInfo(model, primitive);

		MaterialPushConstantData push{};
		push.nodeIndex = node.nodeIndex;
		push.jointOffset = node.jointOffset;
		push.jointCount = node.jointCount;
		push.materialIndex = primitive.materialIndex;
		push.objectIndex = modelMatrixIndex;

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(MaterialPushConstantData),
			&push
		);
	}
	else {
		SimplePushConstantData push{};
		push.objectIndex = modelMatrixIndex;

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(push),
			&push
		);
	}

	uint32_t firstInstance = (instanceCount < 2) ? 0 : 1;

	vkCmdDrawIndexed(commandBuffer, primitive.indexCount, std::max((uint32_t) 1, instanceCount), primitive.firstIndex, 0, firstInstance);
}

void RenderSystem::drawDepth(VkCommandBuffer& commandBuffer, ModelAsset* model, Primitive& primitive, uint32_t modelMatrixIndex, uint32_t cameraIndex, uint32_t instanceCount = 1) const
{
	const NodeDrawInfo node = nodeDrawInfo(model, primitive);

	DepthPushConstantData push{};
	push.objectIndex = modelMatrixIndex;
	push.nodeIndex = node.nodeIndex;
	push.jointOffset = node.jointOffset;
	push.jointCount = node.jointCount;
	push.indexDepthCamera = cameraIndex;

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(push),
		&push
	);

	// non-instanced objects have instanceCount == 0, still draw them once
	uint32_t firstInstance = (instanceCount < 2) ? 0 : 1;

	vkCmdDrawIndexed(commandBuffer, primitive.indexCount, std::max((uint32_t) 1, instanceCount), primitive.firstIndex, 0, firstInstance);
}

bool RenderSystem::bindPose(VkCommandBuffer& commandBuffer, GameObjectModel& obj, uint16_t frameIndex) const
{
	VkDescriptorSet set = obj.getPoseDescriptorSet(frameIndex);
	if (set == VK_NULL_HANDLE) return false;

	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		modelDescriptorSetIndex + 1, 
		1, 
		&set, 
		0,
		nullptr
	);

	return true;
}

void RenderSystem::renderGameObjects(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, const std::array<FrustumPlane, 6>& frustrumPlanes)
{
	bindGlobalSets(commandBuffer, globalDescriptorSets);
	
	for (auto& obj : frameInfo.listGameObjects)
	{
		if (obj->show && !obj->toBeRemoved && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)
		{
			if (obj->lodModelAssets.size()) 
			{
				auto modelAsset = assets.models().get(obj->lodModelAssets[0]);
				
				if (!Camera::isAABBinFrustrum(modelAsset->getAABB(obj->getTransformMat()), frustrumPlanes)) {
					std::cout << "Object " << obj->getName() << " is outside the frustum and will not be rendered.\n";
					return;
				}

				bindModel(commandBuffer, modelAsset, *obj, frameInfo.frameIndex);

				if (useMaterialBuffer) {
					if (modelAsset->lods[0].descriptorSet.empty()) continue;
					bindMaterialBuffer(commandBuffer, modelAsset, frameInfo.frameIndex);
				}
				
				if (usePoseBuffer && !bindPose(commandBuffer, *obj, frameInfo.frameIndex)) continue;

				for (auto primitive : modelAsset->lods[0].primitives)
				{
					if (!useMaterialBuffer)
						bindTextures(commandBuffer, modelAsset, primitive, frameInfo.frameIndex);
					
					drawModel(commandBuffer, modelAsset, primitive, obj->gpuObjectIndex,obj->getInstanceCount());
				}
			}
		}
	}
}

void RenderSystem::renderGameObjectsDepth(VkCommandBuffer& commandBuffer, FrameInfo& frameInfo, std::vector<VkDescriptorSet> globalDescriptorSets, int lightIndex, uint16_t frameIndex)
{ 
	bindGlobalSets(commandBuffer, globalDescriptorSets);

	for (auto obj : frameInfo.listGameObjects)
	{
		if (obj->show && !obj->toBeRemoved && obj->getModelType() == modelType && obj->getModelSubType() == modelSubType)
			if (obj->lodModelAssets.size())
			{
				auto modelAsset = assets.models().get(obj->lodModelAssets[0]);

				if (!modelAsset->hasShadow) continue;
				//if (!Camera::isAABBinFrustrum(obj->getAABB().getAABB(obj->getTransformMat()), frameInfo.listFrustrumPlanes[lightIndex])) continue;


				bindModel(commandBuffer, modelAsset, *obj, frameInfo.frameIndex);

				if (usePoseBuffer && !bindPose(commandBuffer, *obj, frameInfo.frameIndex)) continue;

				for (auto primitive : modelAsset->lods[0].primitives)
				{
					drawDepth(commandBuffer, modelAsset, primitive, obj->gpuObjectIndex, lightIndex, obj->getInstanceCount());
				}
			}
	}
}

void RenderSystem::renderFullScreen(VkCommandBuffer& commandBuffer, std::vector<VkDescriptorSet> globalDescriptorSets, glm::mat4 view, glm::mat4 proj)
{

	bindGlobalSets(commandBuffer, globalDescriptorSets);

	CubeMapPushConstantData push;
	push.view = view;
	push.proj = proj;

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
	
	// draw fullscreen triangle
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}
