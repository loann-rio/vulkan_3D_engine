#include "Renderer.h"

#include "Swapchain.h"
#include "FrameContext.h"


#include "../base/Device.h"

#include <stdexcept>
#include <cassert>

GlobalRenderer::GlobalRenderer(Device& device, Window& window)
    : device(device), window(window) 
{
    recreateSwapchain();
    createSemaphore();
    createFrameRenderer();
}


GlobalRenderer::~GlobalRenderer() {
    device.waitIdle();

    if (timelineSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device.device(), timelineSemaphore, nullptr);
    }
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

    frameContext.swapchain = swapchain.get();
}

void GlobalRenderer::createSemaphore()
{
    VkSemaphoreTypeCreateInfo timelineInfo{};
    timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = 0;

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = &timelineInfo;

    if (vkCreateSemaphore(device.device(), &createInfo, nullptr, &timelineSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create timeline semaphore");
    }
}

void GlobalRenderer::createFrameRenderer()
{
    frameRenderer = std::make_unique<FrameRenderer>();

    /*auto mainPass = std::make_unique<MainPass>(device,
        mainRenderPass,
        swapchain->extent()
    );

    frameRenderer->addPass(std::move(mainPass));*/

  
}

void GlobalRenderer::renderFrame() 
{
    
    frameContext.frameIndex = currentFrameIndex;
    frameContext.timeline = timelineSemaphore;
    frameContext.timelineValue = timelineValue;

    if (!aquireFrame()) {
        return;
    }

    frameRenderer->render(frameContext);

    // wait for all passes to complete
    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &timelineSemaphore;
    waitInfo.pValues = &frameContext.timelineValue;

    vkWaitSemaphores(device.device(), &waitInfo, UINT64_MAX);


    presentFrame();

    timelineValue = frameContext.timelineValue;
    currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
    
}

bool GlobalRenderer::aquireFrame()
{
    vkWaitForFences(device.device(), 1, &frameContext.inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result = swapchain->acquireNextImage(
        frameContext.frameIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    vkResetFences(device.device(), 1, &frameContext.inFlightFence);
    return true;
}

void GlobalRenderer::presentFrame()
{

    VkResult result = swapchain->present(frameContext.frameIndex, timelineSemaphore, frameContext.timelineBaseValue);

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        window.wasWindowResized()) {

        window.resetWindowResizedFlag();
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present frame");
    }
}
