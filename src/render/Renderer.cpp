#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include "../model/ModelAsset.h"
#include "../model/ModelBuilder.h"
#include "../assetManager/ModelManager.h"

Renderer::Renderer(Window& window, Device& device, AssetManager& assets) : window{window} , device{device}, assets{assets}
{
	recreateSwapChain();

	isDepthStarted.resize(DepthSwapChain::MAX_DEPTH_RENDER_COUNT);
	depthSwapChain = std::make_unique<DepthSwapChain>(device, assets, VkExtent2D{ 2048, 2048 });

	createDepthCommandBuffer();
	createCommandBuffer();

	//imgui = std::make_unique<BasicUI>(device, assets, window.getGLFWwindow(), getSwapChainRenderPass() );
}

Renderer::~Renderer() { freeCommandBuffers(); }

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
	if (swapChain == nullptr) {
		swapChain = std::make_unique<Swap_chain>(device, assets, extent);
	}
	else {
		std::shared_ptr<Swap_chain> oldSwapChain = std::move(swapChain);
		swapChain = std::make_unique<Swap_chain>(device, assets, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormat(*swapChain.get())) {
			throw std::runtime_error("Swap chain image format as changed");
		}
	}
}


/// <summary>
/// create command buffer
/// </summary>
/// <returns></returns>
VkCommandBuffer Renderer::beginFrame()
{
	assert(!isFrameStarted && "can't call beginframe while a frame is already in progress"); 
	 
	isFrameStarted = true; 

	auto commandBuffer = getCurrentCommandBuffer(); 

	vkResetCommandBuffer(commandBuffer, 0); 

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer");
	}

	return commandBuffer;
}

void Renderer::endFrame()
{
	assert(isFrameStarted && "cant call endFrame while the frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer");
	}

	VkResult result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
		window.resetWindowResizedFlag();
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false; 
	currentFrameIndex = (currentFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;
}

VkCommandBuffer Renderer::beginDepthFrame(int depthCommandBufferIndex)
{
	assert(!isDepthStarted[depthCommandBufferIndex] && "can't call beginframe while a frame is already in progress");

	isDepthStarted[depthCommandBufferIndex] = true;
	auto commandBuffer = getCurrentDepthCommandBuffer(depthCommandBufferIndex);

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{}; 
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { 
		throw std::runtime_error("failed to begin recording command buffer"); 
	}

	return commandBuffer; 
}

void Renderer::endDepthFrame(int depthCommandBufferIndex)
{
	assert(isDepthStarted[depthCommandBufferIndex] && "cant call endFrame while the frame is not in progress");
	auto commandBuffer = getCurrentDepthCommandBuffer(depthCommandBufferIndex);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer");
	}
}

void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(isFrameStarted && "cant call beginSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "cant begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapChain->getRenderPass();
	renderPassInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.23f, 0.5f, 0.92f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(swapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, swapChain->getSwapChainExtent() };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::beginShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex)
{
	assert(isDepthStarted[depthCommandBufferIndex] && "cant call beginSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == getCurrentDepthCommandBuffer(depthCommandBufferIndex) && "cant begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = depthSwapChain->getDepthRenderPass();
	renderPassInfo.framebuffer = depthSwapChain->getDepthFramebuffers(depthCommandBufferIndex + currentDepthFrameIndex * DepthSwapChain::MAX_DEPTH_RENDER_COUNT);

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
	assert(isFrameStarted && "cant call endSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "cant end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

void Renderer::endShadowRenderPass(VkCommandBuffer commandBuffer, int depthCommandBufferIndex)
{
	assert(isDepthStarted[depthCommandBufferIndex] && "cant call endSwapChainRenderPass while frame not in progress");
	assert(commandBuffer == getCurrentDepthCommandBuffer(depthCommandBufferIndex) && "cant end render pass on command buffer from a different frame");

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

void Renderer::endSingleTimeRender(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

bool Renderer::aquireNextImage()
{
	auto result = swapChain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return false;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image");
	}

	return true;
}

void Renderer::renderDepthImage(FrameInfo& frameInfo, std::vector<std::shared_ptr<GlobalRenderSystem>> renderSystems, std::vector<VkDescriptorSet> globalDescriptorSets)
{

	size_t countDepthRender = 0; 
	for (int commandBufferIndex = 0; commandBufferIndex < DepthSwapChain::MAX_DEPTH_RENDER_COUNT && commandBufferIndex < frameInfo.spotLightCount; commandBufferIndex++)
	{
		if (auto depthCommandBuffer = beginDepthFrame(commandBufferIndex)) {

			beginShadowRenderPass(depthCommandBuffer, commandBufferIndex); 

			for (auto renderSystem : renderSystems)
				renderSystem->renderGameObjectsDepth(depthCommandBuffer, frameInfo, globalDescriptorSets,  commandBufferIndex, frameInfo.frameIndex);

			endShadowRenderPass(depthCommandBuffer, commandBufferIndex);
			endDepthFrame(commandBufferIndex);
			countDepthRender++;
		}
	}


	swapChain->submitDepthCommandBuffer(getCurrentDepthCommandBuffers(countDepthRender));

	for (uint32_t i = 0; i < isDepthStarted.size(); i++) 
		isDepthStarted[i] = false;  

	currentDepthFrameIndex = (currentDepthFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT; 
}

void Renderer::renderColorImage(
	ObjectManager& objectManager,
	FrameInfo& frameInfo,
	std::vector<VkDescriptorSet> globalDescriptorSet,
	std::vector<VkDescriptorSet> shadowDescriptorSet,
	std::vector<VkDescriptorSet> terrainDescriptorSet,
	GlobalRenderSystem* gltfRenderSystem,
	GlobalRenderSystem* objRenderSystem,
	GlobalRenderSystem* terrainRenderSystem,
	GlobalRenderSystem* skyboxRenderSystem)
{
	if (auto commandBuffer = beginFrame()) {

		// render
		beginSwapChainRenderPass(commandBuffer);

		if (base_skybox)
			gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo,
				{
					globalDescriptorSet[frameInfo.frameIndex],
					shadowDescriptorSet[getDepthIndex()],
					assets.models().get(base_skybox->modelAsset)->lods[0].materials[0].descriptorSet[frameInfo.frameIndex]
				},
				frameInfo.mainCameraFrustrumPlanes);
		else
			base_skybox = dynamic_cast<GameObjectModel*>(objectManager.get("cubemap1"));


		objRenderSystem->renderGameObjects(commandBuffer, frameInfo, {globalDescriptorSet[frameInfo.frameIndex], shadowDescriptorSet[frameInfo.frameIndex] });

		//std::vector<VkDescriptorSet> terrainDescriptorSet{ globalDescriptorSet[frameInfo.frameIndex], shadowDescriptorSet[getDepthIndex()], terrainDescriptorSet[frameInfo.frameIndex] };
		//terrainRenderSystem->renderGameObjects(commandBuffer, frameInfo, terrainDescriptorSet);

		skyboxRenderSystem->renderGameObjects(commandBuffer, frameInfo, { globalDescriptorSet[frameInfo.frameIndex] });

		//imgui->drawUI(commandBuffer, &objectManager, terrainUbo, frameInfo.gpuFrameRate);

		endSwapChainRenderPass(commandBuffer);
		endFrame();
	}
}

void Renderer::generateSkybox(const std::string pathTexture, const std::string goName, ObjectManager& objectManager, std::shared_ptr<GlobalRenderSystem> skyboxRenedrSystem)
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
	auto resultTexture = renderHdriToCubeTexture(skyboxRenedrSystem, descriptorSet);


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
	ModelManager::ModelID modelId = assets.models().create(modelBuilder.fromFile("model/cube.obj").withTexture(resultTexture));

	objectManager.createDescriptorSet(assets.models().get(modelId));
	gameObject->setModel(modelId);

	objectManager.pushGameObject(std::move(gameObject));
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


			endSingleTimeRender(commandBuffer);
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


void Renderer::createCommandBuffer()
{
	commandBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getThreadCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());


	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffer");
	}
}

void Renderer::createDepthCommandBuffer()
{
	depthCommandBuffers.resize(DepthSwapChain::MAX_DEPTH_RENDER_COUNT * Swap_chain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getThreadCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(depthCommandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, depthCommandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffer");
	}
}

void Renderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(device.device(), device.getThreadCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();

	vkFreeCommandBuffers(device.device(), device.getThreadCommandPool(), static_cast<uint32_t>(depthCommandBuffers.size()), depthCommandBuffers.data());
	depthCommandBuffers.clear();
}

