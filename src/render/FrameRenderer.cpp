#include "FrameRenderer.h"

#include "../base/device.h"
#include "../base/Swap_chain.h"

#include <cassert>
#include <stdexcept>

FrameRenderer::FrameRenderer(Device& device, Swap_chain* swapchain) : swapchain{swapchain}, device{device}
{
	createSyncObjects();
	createCommandBuffers();
}

FrameRenderer::~FrameRenderer()
{
	destroySyncObjects();
}

VkCommandBuffer FrameRenderer::beginFrame()
{
	assert(!frameStarted && "can't call beginframe while a frame is already in progress");

	frameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer");
	}

	return commandBuffer;
}

VkResult FrameRenderer::endFrame()
{
	assert(frameStarted && "cant call endFrame while the frame is not in progress");
	auto commandBuffer = getCurrentCommandBuffer();

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer");
	}

	VkResult result = swapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

	frameStarted = false;
	currentFrameIndex = (currentFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;

	return result;
}

void FrameRenderer::recreate(Swap_chain* swapchain)
{
	this->swapchain = swapchain;
}

VkCommandBuffer FrameRenderer::getCurrentCommandBuffer() const
{
	assert(frameStarted && "cannot get command buffer when frame not in progress");
	return commandBuffers[currentFrameIndex];
}

uint32_t* FrameRenderer::getCurrentImageIndex()
{
	return &currentImageIndex;
}

uint32_t FrameRenderer::getCurrentFrameIndex() const
{
	return currentFrameIndex;
}

bool FrameRenderer::isFrameInProgress() const
{
	return frameStarted;
}

void FrameRenderer::createCommandBuffers()
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

void FrameRenderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(device.device(), device.getThreadCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();
}

void FrameRenderer::createSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &frames[i].imageAvailableSemaphore) !=
			VK_SUCCESS ||
			vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &frames[i].renderFinishedSemaphore) !=
			VK_SUCCESS ||
			vkCreateFence(device.device(), &fenceInfo, nullptr, &frames[i].inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void FrameRenderer::destroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device.device(), frames[i].renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device.device(), frames[i].imageAvailableSemaphore, nullptr);
		vkDestroyFence(device.device(), frames[i].inFlightFence, nullptr);
	}
}
