#include "Renderer.h"

#include "Swapchain.h"
#include "FrameContext.h"


#include "../base/Device.h"
#include "RenderPass/MainPass.h"

#include "RenderSystems/RenderSystemBuilder.h"
#include "../model/Vertex/ObjVertexData.h"

#include <stdexcept>
#include <cassert>

GlobalRenderer::GlobalRenderer(Device& device, Window& window, AssetManager& assetManager, ObjectManager& objectManager)
	: device(device), window(window), assetManager(assetManager), objectManager(objectManager)
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


/// recreate swap chain after redimentionning of the window
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
    frameRenderer = std::make_unique<FrameRenderer>(device);

	auto mainPass = std::make_unique<MainPass>(
		device, assetManager, *swapchain,
        Swapchain::MAX_FRAMES_IN_FLIGHT,
		swapchain->getExtent()
    );

    auto baseRenderSystem = RenderSystemBuilder()
        .fragmentShader("MainMeshShader.frag.spv")
        .vertexShader("MainMeshShader.vert.spv")
        .cullMode(VK_CULL_MODE_FRONT_AND_BACK)
        .renderPass(mainPass->getRenderPass())
        .vertexLayout(new ObjVertexLayout)
        .buildMain(device, assetManager);

    mainPass->addRenderSystem(
        std::move(baseRenderSystem)
	);

    frameRenderer->addPass(std::move(mainPass));
}

bool GlobalRenderer::aquireFrame()
{
    uint32_t imageIndex = 0;

    uint32_t frameSlot = frameContext.frameIndex;

    VkResult result = swapchain->acquireNextImage(
        &imageIndex,
        &frameSlot
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // Populate frame context
    frameContext.imageIndex = imageIndex;
    frameContext.imageAvailable = swapchain->getImageAvailableSemaphore(frameSlot);
    frameContext.swapchain = swapchain.get();

    return true;
}


void GlobalRenderer::renderFrame()
{
    const uint64_t frameId = frameCounter;
    const uint32_t frameSlot = frameId % Swapchain::MAX_FRAMES_IN_FLIGHT;

    // Wait only if about to reuse frame slot in use by GPU
    uint64_t requiredTimeline =
        (frameId >= Swapchain::MAX_FRAMES_IN_FLIGHT)
        ? frameId - Swapchain::MAX_FRAMES_IN_FLIGHT
        : 0;

    if (requiredTimeline > lastCompletedFrame)
    {
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &timelineSemaphore;
        waitInfo.pValues = &requiredTimeline;

        vkWaitSemaphores(device.device(), &waitInfo, UINT64_MAX);
        lastCompletedFrame = requiredTimeline;
    }

	// acquire frame
    frameContext.frameIndex = frameSlot;
    frameContext.timeline = timelineSemaphore;
    frameContext.timelineValue = frameId;

    if (!aquireFrame()) {
        return;
    }

	// record and submit frame 
    frameRenderer->render(frameContext);

	// present frame
    presentFrame();

    frameCounter++;
}

void GlobalRenderer::presentFrame()
{
    VkResult result = swapchain->present(frameContext.frameIndex);

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
