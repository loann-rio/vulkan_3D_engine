#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "../model/ModelAsset.h"
#include "../model/ModelBuilder.h"
#include "../assetManager/ModelManager.h"
#include "../base/device.h"
#include "GlobalRenderSystemBuilder.h"
#include "../model/Vertex/ObjVertexData.h"
#include "../model/Vertex/GlTFVertexData.h"



Renderer::Renderer(
	Window& window, 
	Device& device, 
	AssetManager& assets, 
	ObjectManager& objectManager) 
	: window{window}, device{device}, assets{assets}
{
	recreateSwapChain(objectManager);

	createPasses();
	initUi();

	createTextureTarget();
	createBuffers(objectManager);
	createTimestampPool();
}

Renderer::~Renderer()
{
	vkDestroyQueryPool(device.device(), timestampPool, nullptr);
}

void Renderer::createTextureTarget()
{
	// shadow
	depthFrameTarget = std::make_unique<PassTarget>(
		device,
		*swapChain.get(),
		assets,
		VkExtent2D{1024, 1024},	
		true,  /*depth*/
		false, /*color*/
		DepthPass::MAX_DEPTH_RENDER_COUNT * Swap_chain::MAX_FRAMES_IN_FLIGHT,
		false
	);

	// color
	colorFrameTarget = std::make_unique<PassTarget>(
		device,
		*swapChain.get(),
		assets,
		swapChain->getSwapChainExtent(),
		true,
		true,
		Swap_chain::MAX_FRAMES_IN_FLIGHT,
		false
	);	

	// post process
	postPFrameTarget = std::make_unique<PassTarget>(
		device,
		*swapChain.get(),
		assets,
		swapChain->getSwapChainExtent(),
		false,
		true,
		swapChain->imageCount(),
		true
	);

	depthFrameTarget->createLocalFramebuffers(depthPass->getRenderPass());
	colorFrameTarget->createLocalFramebuffers(colorPass->getRenderPass());
	postPFrameTarget->createLocalFramebuffers(postPass->getRenderPass());

	depthPass->setTarget(depthFrameTarget.get());
	colorPass->setTarget(colorFrameTarget.get());
	postPass->setTarget(postPFrameTarget.get());

	swapChain->createFramebuffers(
		assets,
		postPFrameTarget.get(),
		postPass->getRenderPass()
	);
}

void Renderer::createPasses()
{
	depthPass = std::make_unique<DepthPass>(
		device,
		assets,
		swapChain.get()
	);

	colorPass = std::make_unique<ColorPass>(
		device,
		assets,
		swapChain.get()
	);

	postPass = std::make_unique<PostProPass>(
		device,
		assets,
		swapChain.get()
	);

	computePass = std::make_unique<ComputePass>(
		device,
		assets
	);
}

void Renderer::initUi()
{
	imgui = std::make_unique<BasicUI>(
		device,
		assets,
		window.getGLFWwindow(),
		colorPass->getRenderPass()
	);

	colorPass->setUi(imgui.get()); // imgui need to have depth test enable -> not usable in post pro
}

void Renderer::updateModelPositionBuffer(std::vector<GameObjectModel*>& listGameObjects, uint16_t frameIndex)
{
	auto* objects = static_cast<GpuObjectData*>(objectBuffers[frameIndex]->getMappedMemory());
	uint32_t count = 0;
	for (auto* obj : listGameObjects) {
		if (count == MAX_GPU_OBJECTS) break;  
		obj->gpuObjectIndex = count;
		objects[count++].model = obj->getTransformMat();
	}
}

bool Renderer::aquireNextImage(ObjectManager& objectManager)
{
	auto result = swapChain->acquireNextImage(frameRenderer.getCurrentImageIndex());

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain(objectManager);
		return false;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image");
	}

	return true;
}

void Renderer::beginSingleTimeRender(VkCommandBuffer commandBuffer, uint16_t buffer_index)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = skyboxSwapChain.getRenderPass();
	renderPassInfo.framebuffer = skyboxSwapChain.getFrameBuffer(buffer_index);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = skyboxSwapChain.getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.23f, 0.5f, 0.92f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(skyboxSwapChain.getSwapChainExtent().width);
	viewport.height = static_cast<float>(skyboxSwapChain.getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, skyboxSwapChain.getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::generateSkybox(const std::string pathTexture, const std::string goName, ObjectManager& objectManager)
{
	auto textureSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	TextureBuilder builder(device);
	TextureManager::TextureID texture = assets.textures().create(builder.fromFile(pathTexture));

	auto imageInfo = assets.textures().get(texture)->getImageInfo();

	VkDescriptorSet descriptorSet;
	DescriptorWriter(*textureSetLayout, *objectManager.getPool())
		.writeImage(0, &imageInfo)
		.build(descriptorSet);

	// render new texture
	auto resultTexture = renderHdriToCubeTexture(skyboxCreationRenderSystem, descriptorSet);

	// remove builder texture
	assets.textures().remove(texture);

	// create go with new texture
	auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, assets);
	gameObject->setName(goName);

	gameObject->setModelType(ModelType::OBJ_MODEL);
	gameObject->setModelSubType(ModelSubType::SKYBOX);

	gameObject->texturePath = gameObject->texturePath;
	gameObject->saveable = false;
	gameObject->show = true;

	GameObject::id_t id = gameObject->getId();

	ModelBuilder modelBuilder(device, assets);
	ModelManager::ModelID modelId = assets
		.models()
		.create(modelBuilder
			.fromFile("assets/model/cube.obj")
			.withTexture(resultTexture)
		);

	objectManager.createDescriptorSet(assets.models().get(modelId));
	gameObject->setModel(modelId);

	objectManager.pushGameObject(std::move(gameObject));
}

void Renderer::renderFrame(FrameInfo& frameInfo, ObjectManager& objectManager)
{
	frameInfo.gpuFrameRate = gpuFrameRate.get();

	frameInfo.globalDescriptorSet = globalDescriptorSet;
	frameInfo.shadowDescriptorSet = shadowDescriptorSet;
	frameInfo.terrainDescriptorSet = terrainDescriptorSet;
	frameInfo.postProDescriptorSet = postProDescriptorSet;

	if (aquireNextImage(objectManager))
	{
		frameInfo.frameIndex = frameRenderer.getCurrentFrameIndex();
		frameInfo.imageIndex = *frameRenderer.getCurrentImageIndex();

		for (auto* obj : objectManager.getByType<GameObjectModel>())
			obj->uploadPose(static_cast<uint16_t>(frameInfo.frameIndex));

		updateModelPositionBuffer(frameInfo.listGameObjects, static_cast<uint16_t>(frameInfo.frameIndex));

		if (auto commandBuffer = frameRenderer.beginFrame()) {

			readGpuTime(frameInfo.frameIndex);
			vkCmdResetQueryPool(commandBuffer, timestampPool, frameInfo.frameIndex * 2, 2);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestampPool, frameInfo.frameIndex * 2);

			depthPass->recordPass(
				objectManager,
				frameInfo,
				commandBuffer
			);
			
			for (ComputeObject computeObject : objectManager.computeList) {
				computePass->recordPass(
					frameInfo,
					commandBuffer,
					computeObject
				);
			}
			
			colorPass->recordPass(
				objectManager,
				frameInfo,
				commandBuffer
			);

			postPass->recordPass(
				objectManager,
				frameInfo,
				commandBuffer
			);

			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestampPool, frameInfo.frameIndex * 2 + 1);
			timestampWritten[frameInfo.frameIndex] = true;

			VkResult result = frameRenderer.endFrame();

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
				window.resetWindowResizedFlag();
				recreateSwapChain(objectManager);
			}
			else if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image");
			}
		}
	}
}

TextureManager::TextureID Renderer::renderHdriToCubeTexture(std::shared_ptr<RenderSystem> renderSystem, VkDescriptorSet descriptorSet)
{

	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0), glm::vec3(1,  0,  0), glm::vec3(0, -1,  0)), // +X
		glm::lookAt(glm::vec3(0), glm::vec3(-1, 0,  0), glm::vec3(0, -1,  0)), // -X
		glm::lookAt(glm::vec3(0), glm::vec3(0, -1,  0), glm::vec3(0,  0, -1)), // +Y
		glm::lookAt(glm::vec3(0), glm::vec3(0,  1,  0), glm::vec3(0,  0,  1)), // -Y
		glm::lookAt(glm::vec3(0), glm::vec3(0,  0,  1), glm::vec3(0, -1,  0)), // +Z
		glm::lookAt(glm::vec3(0), glm::vec3(0,  0, -1), glm::vec3(0, -1,  0))  // -Z
	};

	glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	captureProj[1][1] *= -1.0f;

	for (size_t face = 0; face < 6; face++)
	{
		if (auto commandBuffer = device.beginSingleTimeCommands()) {
			beginSingleTimeRender(commandBuffer, face);

			renderSystem->renderFullScreen(commandBuffer, { descriptorSet }, captureViews[face], captureProj);

			vkCmdEndRenderPass(commandBuffer);
			device.endSingleTimeCommands(commandBuffer);
		}

	}

	device.transitionImageLayout(
		assets.textures().get(skyboxSwapChain.getTextureColor())->image(),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		1,
		6
	);

	return skyboxSwapChain.getTextureColor();
}

void Renderer::createTimestampPool()
{
	VkQueryPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount = 2 * Swap_chain::MAX_FRAMES_IN_FLIGHT;

	if (vkCreateQueryPool(device.device(), &info, nullptr, &timestampPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create timestamp query pool");
	}
}

void Renderer::readGpuTime(uint32_t frameIndex)
{
	if (!timestampWritten[frameIndex]) return;

	uint64_t ts[2];
	VkResult result = vkGetQueryPoolResults(
		device.device(), 
		timestampPool,
		frameIndex * 2,
		2,
		sizeof(ts),
		ts,
		sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT
	);

	if (result == VK_SUCCESS) {
		gpuTime = static_cast<float>(ts[1] - ts[0]) * device.properties.limits.timestampPeriod * 1e-9f;
		gpuFrameRate.update(gpuTime);
	}
}

void Renderer::createBuffers(ObjectManager& objectManager)
{
	/// global buffer + object matrix buffer
	uboBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < uboBuffers.size(); i++)
	{
		uboBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			device.properties.limits.minUniformBufferOffsetAlignment
		);

		uboBuffers[i]->map();
	}

	objectBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < objectBuffers.size(); i++)
		{
			objectBuffers[i] = std::make_unique<Buffer>(
				device,
				sizeof(GpuObjectData) * MAX_GPU_OBJECTS,
				1,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				device.properties.limits.minUniformBufferOffsetAlignment
			);

			objectBuffers[i]->map();
		}

	auto globalSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build();

	globalDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSet.size(); i++)
	{
		auto bufferInfo = uboBuffers[i]->descriptorInfo();
		auto objectInfo = objectBuffers[i]->descriptorInfo();

		DescriptorWriter(*globalSetLayout, *objectManager.getPool())
			.writeBuffer(0, &bufferInfo)
			.writeBuffer(1, &objectInfo)
			.build(globalDescriptorSet[i]);
	}

	//// terrain buffer 
	terrainBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < terrainBuffers.size(); i++)
	{
		terrainBuffers[i] = std::make_unique<Buffer>(
			device,
			sizeof(TerrainUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			device.properties.limits.minUniformBufferOffsetAlignment
		);

		terrainBuffers[i]->map();
	}

	auto terrainSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	terrainDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < terrainDescriptorSet.size(); i++)
	{
		auto bufferInfo = terrainBuffers[i]->descriptorInfo();

		DescriptorWriter(*terrainSetLayout, *objectManager.getPool())
			.writeBuffer(0, &bufferInfo)
			.build(terrainDescriptorSet[i]);
	}

	//// shadow buffer
	shadowUboBuffer.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < shadowUboBuffer.size(); i++)
	{
		shadowUboBuffer[i] = std::make_unique<Buffer>(
			device,
			sizeof(SpotLightUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			device.properties.limits.minUniformBufferOffsetAlignment
		);

		shadowUboBuffer[i]->map();
	}

	auto shadowSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, DepthPass::MAX_DEPTH_RENDER_COUNT)
		.build();

	shadowDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < shadowDescriptorSet.size(); i++)
	{
		auto bufferInfo = shadowUboBuffer[i]->descriptorInfo();

		std::array<VkDescriptorImageInfo, DepthPass::MAX_DEPTH_RENDER_COUNT> imagesInfo;
		for (uint16_t depthImageIndex = 0; depthImageIndex < DepthPass::MAX_DEPTH_RENDER_COUNT; depthImageIndex++) 
		{
			uint16_t depthIndex = i * DepthPass::MAX_DEPTH_RENDER_COUNT + depthImageIndex;
			imagesInfo[depthImageIndex] = depthFrameTarget->getDepthImageInfo(depthIndex);
		}

		DescriptorWriter(*shadowSetLayout, *objectManager.getPool())
			.writeBuffer(0, &bufferInfo)
			.writeImage(1, imagesInfo.data(), DepthPass::MAX_DEPTH_RENDER_COUNT)
			.build(shadowDescriptorSet[i]);
	}

	auto postSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();


	postProDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < postProDescriptorSet.size(); i++) {
		auto imageInfo = colorFrameTarget->getColorImageInfo(i);
		DescriptorWriter(*postSetLayout, *objectManager.getPool())
			.writeImage(0, &imageInfo)
			.build(postProDescriptorSet[i]);
	}

	/// skybox 
	auto skyboxSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	skyboxCreationRenderSystem = GlobalRenderSystemBuilder(device, assets)
		.vertexShader("shaders\\fullscreen.vert.spv")
		.fragmentShader("shaders\\equirectangular_to_cube.frag.spv")
		.renderPass(skyboxSwapChain.getRenderPass())
		.modelFilterType(ModelType::OBJ_MODEL)
		.fullscreen()
		.pushStage(static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_FRAGMENT_BIT))
		.build<ObjVertexLayout>();
}

void Renderer::recreateSwapChain(ObjectManager& objectManager)
{
	// get size window
	auto extent = window.getExtent();

	// if the size of the window is null, wait for the user to modify it to an allowed value
	while (extent.width == 0 || extent.height == 0) {
		extent = window.getExtent();
		glfwWaitEvents();
	}

	// wait for the device to render previous frame
	vkDeviceWaitIdle(device.device());

	// if the swapchain does not already exist create a new one
	if (swapChain == nullptr)
	{
		swapChain = std::make_unique<Swap_chain>(device, assets, extent);
	}
	else
	{
		std::shared_ptr<Swap_chain> oldSwapChain = std::move(swapChain);
		swapChain = std::make_unique<Swap_chain>(device, assets, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormat(*swapChain.get())) {
			throw std::runtime_error("Swap chain image format as changed");
		}
	}

	if (depthFrameTarget) {
		depthFrameTarget->resizeTargets(
			extent, 
			DepthPass::MAX_DEPTH_RENDER_COUNT * Swap_chain::MAX_FRAMES_IN_FLIGHT, 
			swapChain->getSwapChainImageFormat(), 
			swapChain->getSwapChainDepthFormat()
		);

		colorFrameTarget->resizeTargets(
			extent,
			Swap_chain::MAX_FRAMES_IN_FLIGHT,
			swapChain->getSwapChainImageFormat(),
			swapChain->getSwapChainDepthFormat()
		);

		postPFrameTarget->resizeTargets(
			extent,
			swapChain->imageCount(),
			swapChain->getSwapChainImageFormat(),
			swapChain->getSwapChainDepthFormat()
		);

		depthFrameTarget->createLocalFramebuffers(depthPass->getRenderPass());
		colorFrameTarget->createLocalFramebuffers(colorPass->getRenderPass());
		postPFrameTarget->createLocalFramebuffers(postPass->getRenderPass());

		swapChain->createFramebuffers(
			assets,
			postPFrameTarget.get(),
			postPass->getRenderPass()
		);

		createBuffers(objectManager);
	}
	
	frameRenderer.recreate(swapChain.get());
}