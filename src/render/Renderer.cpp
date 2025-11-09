#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <cassert>

Renderer::Renderer(Window& window, Device& device) : window{window} , device{device}
{
	isDepthStarted.resize(DepthSwapChain::MAX_DEPTH_RENDER_COUNT);
	recreateSwapChain();
	depthSwapChain = std::make_unique<DepthSwapChain>(device, VkExtent2D{ 2048, 2048 });

	/*SwapChainBuilder builder{};
	builder.colorTarget = target;
	builder.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	builder.windowExtent = { 1024, 1024 };

	skyboxRenderSwapchain = std::make_unique<SecondarySwapchain>(device, builder);*/

	createDepthCommandBuffer();
	createCommandBuffer();
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
		swapChain = std::make_unique<Swap_chain>(device, extent);
	}
	else {
		std::shared_ptr<Swap_chain> oldSwapChain = std::move(swapChain);
		swapChain = std::make_unique<Swap_chain>(device, extent, oldSwapChain);

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


void Renderer::beginSingleTimeRender(VkCommandBuffer commandBuffer)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swap.getRenderPass();
	renderPassInfo.framebuffer = swap.getFrameBuffer();

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swap.getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.23f, 0.5f, 0.92f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap.getSwapChainExtent().width);
	viewport.height = static_cast<float>(swap.getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, swap.getSwapChainExtent() };
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

std::shared_ptr<Texture> Renderer::renderSingleTotexture(std::shared_ptr<GlobalRenderSystem> renderSystem, GameObjectModel* textureObject, std::vector<VkDescriptorSet> descriptorSets)
{

	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)), // +X
		glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),// -X
		glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),  // +Y
		glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),// -Y
		glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)), // +Z
		glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)) // -Z
	};
		 
	glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	captureProj[1][1] *= -1.0f;

	if (auto commandBuffer = device.beginSingleTimeCommands()) {
		beginSingleTimeRender(commandBuffer);

		FrameInfo info{};
		info.listGameObjects = { textureObject };

		//renderSystem->renderGameObjects(commandBuffer, info, descriptorSets);
		renderSystem->renderFullScreen(commandBuffer, textureObject->getDescriptorSets(), captureViews[0], captureProj);


		endSingleTimeRender(commandBuffer);
		device.endSingleTimeCommands(commandBuffer);
	}
		
	return swap.getTextureColor();

	//// create view + proj matrices
	//glm::mat4 captureViews[] = {
	//	glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)), // +X
	//	glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),// -X
	//	glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),  // +Y
	//	glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),// -Y
	//	glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)), // +Z
	//	glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)) // -Z
	//};
     //
	//glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	//captureProj[1][1] *= -1.0f;
	//
	//// create render image view
	//std::vector<VkImageView> cubeFaceViews{ 6 };
	//for (uint32_t face = 0; face < 6; face++) {
	//	VkImageViewCreateInfo viewInfo{};
	//	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	//	viewInfo.image = textTarget->getImage();
	//	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; 
	//	viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	//	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	viewInfo.subresourceRange.baseMipLevel = 0;
	//	viewInfo.subresourceRange.levelCount = 1;
	//	viewInfo.subresourceRange.baseArrayLayer = face;
	//	viewInfo.subresourceRange.layerCount = 1;
	//
	//	vkCreateImageView(device.device(), &viewInfo, nullptr, &cubeFaceViews[face]);
	//}
	//
	//// create swapchain
	//SwapChainBuilder builder{};
	//
	//builder.additionalImageView = cubeFaceViews;
	//builder.windowExtent = { 1000, 1000 };
	//builder.imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	//
	//VkClearValue clearValues[2];
	//uint32_t clearCount = 0;
	//
	//if (builder.colorTarget || !builder.additionalImageView.empty()) {
	//	clearValues[clearCount].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	//	clearCount++;
	//}
	//
	//if (builder.depthTarget) {
	//	clearValues[clearCount].depthStencil = { 1.0f, 0 };
	//	clearCount++;
	//}
	//
	//// view port and scissor
	//VkViewport viewport{};
	//viewport.x = 0.0f;
	//viewport.y = 0.0f;
	//viewport.width = static_cast<float>(builder.windowExtent.width);
	//viewport.height = static_cast<float>(builder.windowExtent.height);
	//viewport.minDepth = 0.0f;
	//viewport.maxDepth = 1.0f;
	//VkRect2D scissor{ {0, 0}, builder.windowExtent };
	//
	//auto transitionFaceToColor = [&](VkCommandBuffer cmd, uint32_t face) {
	//	VkImageMemoryBarrier barrier{};
	//	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	//	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // or current layout if known
	//	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	barrier.image = textTarget->getImage(); // whole image - but we restrict layers below
	//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	barrier.subresourceRange.baseMipLevel = 0;
	//	barrier.subresourceRange.levelCount = 1;
	//	barrier.subresourceRange.baseArrayLayer = face;
	//	barrier.subresourceRange.layerCount = 1;
	//	barrier.srcAccessMask = 0;
	//	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//
	//	vkCmdPipelineBarrier(
	//		cmd,
	//		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	//		0,
	//		0, nullptr,
	//		0, nullptr,
	//		1, &barrier
	//	);
	//	};
	//
	//auto transitionFaceToShaderRead = [&](VkCommandBuffer cmd, uint32_t face) {
	//	VkImageMemoryBarrier barrier{};
	//	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	//	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	barrier.image = textTarget->getImage();
	//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	barrier.subresourceRange.baseMipLevel = 0;
	//	barrier.subresourceRange.levelCount = 1;
	//	barrier.subresourceRange.baseArrayLayer = face;
	//	barrier.subresourceRange.layerCount = 1;
	//	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	//
	//	vkCmdPipelineBarrier(
	//		cmd,
	//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	//		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	//		0,
	//		0, nullptr,
	//		0, nullptr,
	//		1, &barrier
	//	);
	//	};
	//
	//// render all 6 faces:
	//SecondarySwapchain swap{ device, builder };
	//
	//for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
	//	
	//	auto commandBuffer = device.beginSingleTimeCommands();
	//
	//	VkRenderPassBeginInfo renderPassInfo{};
	//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//	renderPassInfo.renderPass = swap.getRenderPass();
	//	renderPassInfo.framebuffer = swap.getFrameBuffer(faceIndex);
	//	renderPassInfo.renderArea.offset = { 0, 0 };
	//	renderPassInfo.renderArea.extent = builder.windowExtent;
	//	renderPassInfo.clearValueCount = clearCount;
	//	renderPassInfo.pClearValues = clearValues;
	//
	//	transitionFaceToColor(commandBuffer, faceIndex);
	//
	//	// Then start recording
	//	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	//	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	//	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	//
	//	// render here
	//	renderSystem->renderFullScreen(commandBuffer, textureObject->getDescriptorSets(), captureViews[faceIndex], captureProj);
	//
	//	vkCmdEndRenderPass(commandBuffer);
	//
	//	transitionFaceToShaderRead(commandBuffer, faceIndex);
	//
	//	device.endSingleTimeCommands(commandBuffer);
	//
	//	vkQueueWaitIdle(device.presentQueue());
	//}
	//
	//textTarget->recreateImageView(true, true);
	//
	//{
	//	uint32_t layers = textTarget->getCreatedArrayLayers();
	//	VkImageCreateFlags flags = textTarget->getCreatedImageFlags();
	//	bool cubeFlag = (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0;
	//	std::cerr << "[Renderer] cubemap image layers = " << layers << ", cubeCompatibleFlag = " << (cubeFlag ? "YES" : "NO") << "\n";
	//}
	//
	//return textTarget;
}


void Renderer::createCommandBuffer()
{
	commandBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
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
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(depthCommandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, depthCommandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffer");
	}
}

void Renderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();

	vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(depthCommandBuffers.size()), depthCommandBuffers.data());
	depthCommandBuffers.clear();
}

