#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "../model/ModelAsset.h"
#include "../model/ModelBuilder.h"
#include "../assetManager/ModelManager.h"
#include "PassTarget.h"


Renderer::Renderer(
	Window& window, 
	Device& device, 
	AssetManager& assets, 
	ObjectManager& objectManager) 
	: window{window}, device{device}, assets{assets}
{
	recreateSwapChain();

	depthSwapChain = std::make_unique<DepthSwapChain>(
		device, 
		VkExtent2D{ 1024, 1024 }
	);
	
	createTextureTarget(objectManager);
	createRenderSystems(objectManager);

	imgui = std::make_unique<BasicUI>(
		device, 
		assets, 
		window.getGLFWwindow(), 
		getSwapChainRenderPass() 
	);
}

/*
	recreate swap chain after redimentionning of the window
*/
void Renderer::recreateSwapChain()
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

	frameRenderer.recreate(swapChain.get());
}

void Renderer::createTextureTarget(ObjectManager& objectManager)
{
	// shadow
	depthFrameTarget = std::make_unique<PassTarget>(
		device,
		*swapChain.get(),
		assets,
		depthSwapChain->getDepthSwapChainExtent(),
		true,  /*depth*/
		false, /*color*/
		depthSwapChain->findDepthFormat(),
		DepthSwapChain::MAX_DEPTH_RENDER_COUNT * Swap_chain::MAX_FRAMES_IN_FLIGHT,
		false
	);

	depthFrameTarget->createLocalFramebuffers(depthSwapChain->getDepthRenderPass());
	depthFrameTarget->createDescriptorSets(*objectManager.getPool());
}


bool Renderer::aquireNextImage()
{
	auto result = swapChain->acquireNextImage(frameRenderer.getCurrentImageIndex());

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return false;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image");
	}

	return true;
}

void Renderer::renderDepthImage(FrameInfo& frameInfo, VkCommandBuffer& commandBuffer)
{

	size_t countDepthRender = 0;
	for (int depthRenderIndex = 0; depthRenderIndex < DepthSwapChain::MAX_DEPTH_RENDER_COUNT && depthRenderIndex < frameInfo.spotLightCount; depthRenderIndex++)
	{

		beginShadowRenderPass(commandBuffer, depthRenderIndex);

		for (auto renderSystem : {
				depthRenderSystem,
				depthRenderSystemGltf,
				depthTerrainRenderSystem
			})

			renderSystem->renderGameObjectsDepth(
				commandBuffer,
				frameInfo,
				{
					globalDescriptorSet[frameRenderer.getCurrentFrameIndex()],
					shadowDescriptorSet[frameRenderer.getCurrentFrameIndex()]
				},
				depthRenderIndex,
				frameRenderer.getCurrentFrameIndex()
			);

		endSwapChainRenderPass(commandBuffer);
	}
}

void Renderer::renderColorImage(
	ObjectManager& objectManager,
	FrameInfo& frameInfo,
	VkCommandBuffer& commandBuffer)
{
	// render
	beginSwapChainRenderPass(commandBuffer, swapChain->getSwapChainExtent());

	if (base_skybox)
		gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo,
			{
				globalDescriptorSet[frameRenderer.getCurrentFrameIndex()],
				shadowDescriptorSet[frameRenderer.getCurrentFrameIndex()],
				assets.models().get(base_skybox->modelAsset)->lods[0].materials[0].descriptorSet[frameRenderer.getCurrentFrameIndex()]
			},
			frameInfo.mainCameraFrustrumPlanes);
	else
		base_skybox = dynamic_cast<GameObjectModel*>(objectManager.get("cubemap1"));


	objRenderSystem->renderGameObjects(commandBuffer, frameInfo, { globalDescriptorSet[frameRenderer.getCurrentFrameIndex()], shadowDescriptorSet[frameRenderer.getCurrentFrameIndex()] });
	skyboxRenderSystem->renderGameObjects(commandBuffer, frameInfo, { globalDescriptorSet[frameRenderer.getCurrentFrameIndex()] });

	imgui->drawUI(commandBuffer, &objectManager, frameInfo.gpuFrameRate);

	endSwapChainRenderPass(commandBuffer);
}


void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent)
{
	assert(frameRenderer.isFrameInProgress() && "cant call beginSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == frameRenderer.getCurrentCommandBuffer() && "cant begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapChain->getRenderPass();
	renderPassInfo.framebuffer = swapChain->getFrameBuffer(*frameRenderer.getCurrentImageIndex());

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.23f, 0.5f, 0.92f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, extent };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = depthSwapChain->getDepthRenderPass();
	renderPassInfo.framebuffer = depthFrameTarget->getFrameBuffer(depthRenderIndex + frameRenderer.getCurrentFrameIndex() * DepthSwapChain::MAX_DEPTH_RENDER_COUNT);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = depthSwapChain->getDepthSwapChainExtent();

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(depthSwapChain->getDepthSwapChainExtent().width);
	viewport.height = static_cast<float>(depthSwapChain->getDepthSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, depthSwapChain->getDepthSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(frameRenderer.isFrameInProgress() && "cant call endSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == frameRenderer.getCurrentCommandBuffer() && "cant end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Renderer::beginSingleTimeRender(VkCommandBuffer commandBuffer, int buffer_index)
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
	ModelManager::ModelID modelId = assets.models().create(modelBuilder.fromFile("assets/model/cube.obj").withTexture(resultTexture));

	objectManager.createDescriptorSet(assets.models().get(modelId));
	gameObject->setModel(modelId);

	objectManager.pushGameObject(std::move(gameObject));
}

void Renderer::renderFrame(FrameInfo& frameInfo, ObjectManager& objectManager)
{
	frameInfo.gpuFrameRate = gpuFrameRate.get();

	auto newGpuTime = std::chrono::high_resolution_clock::now();

	aquireNextImage();

	if (auto commandBuffer = frameRenderer.beginFrame()) {
		renderDepthImage(
			frameInfo,
			commandBuffer
		);

		renderColorImage(
			objectManager,
			frameInfo,
			commandBuffer
		);

		VkResult result = frameRenderer.endFrame();

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
			window.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
	}

	auto endGpuTime = std::chrono::high_resolution_clock::now();
	gpuTime = std::chrono::duration<float, std::chrono::seconds::period>(endGpuTime - newGpuTime).count();
	gpuFrameRate.update(gpuTime);
}

TextureManager::TextureID Renderer::renderHdriToCubeTexture(std::shared_ptr<GlobalRenderSystem> renderSystem, VkDescriptorSet descriptorSet)
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

			renderSystem->renderFullScreen(commandBuffer, descriptorSet, captureViews[face], captureProj);

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

void Renderer::createRenderSystems(ObjectManager& objectManager)
{
	/// global buffer
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

	auto globalSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();

	globalDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSet.size() && i < 2; i++)
	{
		auto bufferInfo = uboBuffers[i]->descriptorInfo();

		DescriptorWriter(*globalSetLayout, *objectManager.getPool())
			.writeBuffer(0, &bufferInfo)
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
	for (int i = 0; i < terrainDescriptorSet.size() && i < 2; i++)
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
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, DepthSwapChain::MAX_DEPTH_RENDER_COUNT)
		.build();

	shadowDescriptorSet.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < shadowDescriptorSet.size() && i < 2; i++)
	{
		auto bufferInfo = shadowUboBuffer[i]->descriptorInfo();

		std::array<VkDescriptorImageInfo, DepthSwapChain::MAX_DEPTH_RENDER_COUNT> imagesInfo;
		for (uint16_t depthImageIndex = 0; depthImageIndex < DepthSwapChain::MAX_DEPTH_RENDER_COUNT; depthImageIndex++)
			imagesInfo[depthImageIndex] = getDepthImageInfo(i * DepthSwapChain::MAX_DEPTH_RENDER_COUNT + depthImageIndex);

		DescriptorWriter(*shadowSetLayout, *objectManager.getPool())
			.writeBuffer(0, &bufferInfo)
			.writeImage(1, imagesInfo.data(), DepthSwapChain::MAX_DEPTH_RENDER_COUNT)
			.build(shadowDescriptorSet[i]);
	}

	/// skybox 
	auto skyboxSetLayout = DescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	/// render systems

	{
		{
			RenderSystemBuilder gltfBuilder{};
			gltfBuilder.fragFilepath = "shaders\\GlTFshader.frag.spv";
			gltfBuilder.vertFilepath = "shaders\\GlTFshader.vert.spv";
			gltfBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout(), skyboxSetLayout->getDescriptorSetLayout() };
			gltfBuilder.renderPass = getSwapChainRenderPass();
			gltfBuilder.hasMultipleInstance = true;

			gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, assets, gltfBuilder);
		}

		{
			RenderSystemBuilder gltfShadowBuilder{};
			gltfShadowBuilder.vertFilepath = "shaders\\shadowmapgltf.vert.spv";
			gltfShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
			gltfShadowBuilder.renderPass = getDepthRenderPass();
			gltfShadowBuilder.hasMultipleInstance = true;

			depthRenderSystemGltf = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, assets, gltfShadowBuilder);
		}
	}

	{
		{
			RenderSystemBuilder objBuilder{};
			objBuilder.fragFilepath = "shaders\\simple_shader.frag.spv";
			objBuilder.vertFilepath = "shaders\\simple_shader.vert.spv";
			objBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
			objBuilder.renderPass = getSwapChainRenderPass();
			objBuilder.hasMultipleInstance = true;

			objRenderSystem = GlobalRenderSystem::create<Model>(device, assets, objBuilder);
		}

		{
			RenderSystemBuilder objShadowBuilder{};
			objShadowBuilder.vertFilepath = "shaders\\shadowmap.vert.spv";
			objShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
			objShadowBuilder.renderPass = getDepthRenderPass();
			objShadowBuilder.hasMultipleInstance = true;

			depthRenderSystem = GlobalRenderSystem::create<Model>(device, assets, objShadowBuilder);
		}
	}

	{
		{
			RenderSystemBuilder terrainBuilder{};

			terrainBuilder.vertFilepath = "shaders\\terrainShader.vert.spv";
			terrainBuilder.fragFilepath = "shaders\\terrainShader.frag.spv";
			terrainBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() , terrainSetLayout->getDescriptorSetLayout() };
			terrainBuilder.renderPass = getSwapChainRenderPass();
			terrainBuilder.hasMultipleInstance = true;
			terrainBuilder.subModelType = ModelSubType::TERRAIN;

			terrainRenderSystem = GlobalRenderSystem::create<Model>(device, assets, terrainBuilder);
		}

		{
			RenderSystemBuilder terrainShadowBuilder{};
			terrainShadowBuilder.vertFilepath = "shaders\\shadowMapTerrain.vert.spv";
			terrainShadowBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
			terrainShadowBuilder.renderPass = getDepthRenderPass();
			terrainShadowBuilder.hasMultipleInstance = true;
			terrainShadowBuilder.subModelType = ModelSubType::TERRAIN;

			depthTerrainRenderSystem = GlobalRenderSystem::create<Model>(device, assets, terrainShadowBuilder);
		}
	}

	{
		RenderSystemBuilder skyboxBuilder{};
		skyboxBuilder.fragFilepath = "shaders\\skybox.frag.spv";
		skyboxBuilder.vertFilepath = "shaders\\skybox.vert.spv";
		skyboxBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout() };
		skyboxBuilder.renderPass = getSwapChainRenderPass();
		skyboxBuilder.subModelType = ModelSubType::SKYBOX;
		skyboxBuilder.isSkyBox = true;
		skyboxRenderSystem = GlobalRenderSystem::create<Model>(device, assets, skyboxBuilder);
	}

	{
		RenderSystemBuilder skyboxBuilder{};
		skyboxBuilder.fragFilepath = "shaders\\equirectangular_to_cube.frag.spv";
		skyboxBuilder.vertFilepath = "shaders\\fullscreen.vert.spv";
		skyboxBuilder.renderPass = getSecondarySwapRenderPass();
		skyboxBuilder.isFullscreenRender = true;
		skyboxBuilder.pushStage = static_cast<VkShaderStageFlagBits>(VK_SHADER_STAGE_FRAGMENT_BIT);
		skyboxCreationRenderSystem = GlobalRenderSystem::create<Model>(device, assets, skyboxBuilder);
	}

}