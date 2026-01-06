#include "Renderer.h"

#include "Swapchain.h"
#include "FrameContext.h"


#include "../base/Device.h"

#include <stdexcept>
#include <cassert>

GlobalRenderer::GlobalRenderer(Device& device, Window& window)
    : device(device), window(window) {

    recreateSwapchain();
    createFrameContexts();
    createCommandBuffers();

    frameRenderer = std::make_unique<FrameRenderer>(device, *swapchain);
}


GlobalRenderer::~GlobalRenderer() {
    device.waitIdle();
    freeCommandBuffers();
}

/// <summary>
/// 	recreate swap chain after redimentionning of the window
/// </summary>
void GlobalRenderer::recreateSwapchain() {
   
    // get size window
    auto extent = window.getExtent();

    // if the size of the window is null, wait for the user to modify it to an allowed value
    while (extent.width == 0 || extent.height == 0) {
        extent = window.getExtent();
        glfwWaitEvents();
    }

    // wait for device to finish frame creation
    device.waitIdle();

    // if the swapchain does not already exist create a new one
    if (swapchain == nullptr) {
        swapchain = std::make_unique<Swapchain>(device, extent);
    }
    else {
        std::shared_ptr<Swapchain> oldSwapChain = std::move(swapchain);
        swapchain = std::make_unique<Swapchain>(device, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormat(*swapchain.get())) {
            throw std::runtime_error("Swap chain image format as changed");
        }
    }
}

void GlobalRenderer::createFrameContexts()
{
    frames.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < frames.size(); ++i) {
        frames[i].frameIndex = i;
        device.createSyncObjects(
            frames[i].imageAvailable,
            frames[i].renderFinished,
            frames[i].inFlight
        );
    }
}

void GlobalRenderer::createCommandBuffers()
{
    presentCommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getThreadCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(presentCommandBuffers.size());


    if (vkAllocateCommandBuffers(
        device.device(), 
        &allocInfo, 
        presentCommandBuffers.data()) != VK_SUCCESS) {

        throw std::runtime_error("failed to allocate command buffer");
    }

    for (uint32_t i = 0; i < frames.size(); ++i) {
        frames[i].commandBuffer = presentCommandBuffers[i];
    }
}

void GlobalRenderer::freeCommandBuffers()
{
    if (!presentCommandBuffers.empty())
    {
        vkFreeCommandBuffers(
            device.device(), 
            device.getThreadCommandPool(), 
            static_cast<uint32_t>(presentCommandBuffers.size()),
            presentCommandBuffers.data()
        );

        presentCommandBuffers.clear();
    }
}

uint32_t GlobalRenderer::frameIndex() const {
    return currentFrameIndex;
}

void GlobalRenderer::renderFrame() 
{
    FrameContext& frame = frames[currentFrameIndex];

    if (!aquireFrame(frame)) {
        return;
    }

    beginFrame(frame);

    frameRenderer->render(frame);

    endFrame(frame);
    presentFrame(frame);

    currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
    
}

bool GlobalRenderer::aquireFrame(FrameContext& frame)
{
    vkWaitForFences(device.device(), 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

    VkResult result = swapchain->acquireNextImage(
        frame.imageAvailable,
        &frame.imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    vkResetFences(device.device(), 1, &frame.inFlight);
    return true;
}

void GlobalRenderer::presentFrame(FrameContext& frame)
{
    VkResult result = swapchain->submitAndPresent(
        frame.commandBuffer,
        frame.imageAvailable,
        frame.renderFinished,
        frame.inFlight,
        frame.imageIndex
    );

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

void GlobalRenderer::beginFrame(FrameContext& frame)
{
    VkCommandBuffer cmd = frame.commandBuffer;
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }
}

void GlobalRenderer::endFrame(FrameContext& frame)
{
    if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}
