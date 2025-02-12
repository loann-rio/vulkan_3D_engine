#include "Renderer.h"

#include <stdexcept>
#include <array>
#include <cassert>


Renderer::Renderer(Window& window, Device& device) : window{window} , device{device}
{
	isDepthStarted.resize(DepthSwapChain::MAX_DEPTH_RENDER_COUNT);
	recreateSwapChain();
	depthSwapChain = std::make_unique<DepthSwapChain>(device, VkExtent2D{ 2048, 2048 });

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

	/*auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
		window.resetWindowResizedFlag();
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}

	isFrameStarted = false;
	currentFrameIndex = (currentFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;*/
}

VkCommandBuffer Renderer::beginDepthFrame(int depthCommandBufferIndex)
{
	assert(!isDepthStarted[depthCommandBufferIndex] && "can't call beginframe while a frame is already in progress");

	isDepthStarted[depthCommandBufferIndex] = true;
	auto commandBuffer = getCurrentDepthCommandBuffer(depthCommandBufferIndex);

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
	clearValues[0].color = { 0.43f, 0.8f, 0.92f, 1.f };
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
	renderPassInfo.framebuffer = depthSwapChain->getDepthFramebuffers(depthCommandBufferIndex + currentDepthFrameIndex * Swap_chain::MAX_FRAMES_IN_FLIGHT);

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

void Renderer::submitCommandBuffers(bool renderDepth)
{

	VkResult result = VK_INCOMPLETE;

	/*if (renderDepth)
	{
		auto depthCommandBuffer = getCurrentDepthCommandBuffers(); 
		auto commandBuffer = getCurrentCommandBuffer(); 

		result = swapChain->submitDepthAndMainCommandBuffers(depthCommandBuffer, &commandBuffer, &currentImageIndex);
	}
	else
	{*/
		auto commandBuffer = getCurrentCommandBuffer();

		result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex, renderDepth);
	//}
	

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
		window.resetWindowResizedFlag();
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image");
	}

	for (uint32_t i = 0; i < isDepthStarted.size(); i++)
		isDepthStarted[i] = false;

	isFrameStarted = false;

	currentFrameIndex = (currentFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;
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

void Renderer::renderDepthImage(FrameInfo& frameInfo, std::shared_ptr<GlobalRenderSystem> renderSystems)
{
	int countDepthRender = 0;
	for (int commandBufferIndex = 0; commandBufferIndex < DepthSwapChain::MAX_DEPTH_RENDER_COUNT; commandBufferIndex++)
	{
		if (auto depthCommandBuffer = beginDepthFrame(commandBufferIndex)) {
			beginShadowRenderPass(depthCommandBuffer, commandBufferIndex); 
			renderSystems->renderGameObjects(depthCommandBuffer, frameInfo, true, commandBufferIndex); 
			endShadowRenderPass(depthCommandBuffer, commandBufferIndex);
			endDepthFrame(commandBufferIndex);
		}
	}

	
	//depthSwapChain->submitDepthCommandBuffer(getCurrentDepthCommandBuffers());
	swapChain->submitDepthCommandBuffer(getCurrentDepthCommandBuffers());

	currentDepthFrameIndex = (currentDepthFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;
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

