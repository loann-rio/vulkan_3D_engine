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
	createUboDescriptorPool();
    
    createFrameRenderer();
}


GlobalRenderer::~GlobalRenderer() {
    device.waitIdle();
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
        swapchain = std::make_unique<Swapchain>(device, assetManager, extent);
    }
    else {
        std::shared_ptr<Swapchain> oldSwapChain = std::move(swapchain);
        swapchain = std::make_unique<Swapchain>(device, assetManager, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormat(*swapchain.get())) {
            throw std::runtime_error("Swap chain image format as changed");
        }
    }

    frameContext.swapchain = swapchain.get();
}

void GlobalRenderer::createUboDescriptorPool()
{
    globalPool = DescriptorPool::Builder(device)
        .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT * 20)
        .addPoolSize(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            Swapchain::MAX_FRAMES_IN_FLIGHT * 20)
        .build();
}

void GlobalRenderer::createFrameRenderer()
{
    frameRenderer = std::make_unique<FrameRenderer>(device);

	auto mainPass = std::make_unique<MainPass>(
		device, assetManager, 
        *swapchain, *globalPool,
        Swapchain::MAX_FRAMES_IN_FLIGHT,
		swapchain->getExtent()
    );

    {
        auto baseRenderSystem = RenderSystemBuilder()
            .fragmentShader("shaders/MainMeshShader.frag.spv")
            .vertexShader("shaders/MainMeshShader.vert.spv")
            .cullMode(VK_CULL_MODE_FRONT_AND_BACK)
            .renderPass(mainPass->getRenderPass())
            .vertexLayout(new ObjVertexLayout)
            .asMainRenderSystem();

        mainPass->addRenderSystem(baseRenderSystem);
    }

    frameRenderer->addPass(std::move(mainPass));

    createFrameBuffers();
}

void GlobalRenderer::createFrameBuffers()
{
    for (size_t i = 0; i < frameRenderer->getPassCount() - 1; ++i) {
		auto& pass = frameRenderer->getPass(i);
		pass.createLocalFramebuffers();
    }

	// final pass uses swapchain framebuffers
	auto& finalPass = frameRenderer->getLastPass();
	swapchain->createFramebuffers(finalPass.getRenderPass());
	finalPass.setAsFinal();
}

bool GlobalRenderer::aquireNextImage()
{
	// wait for the current frame to be finished
	swapchain->waitForImageInFlight(currentFrameIndex);

	// acquire next image from swapchain
    VkResult result = swapchain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    return true;
}


void GlobalRenderer::renderFrame()
{
    if (!aquireNextImage()) {
        return;
    }

	// record passes 
    frameRenderer->recordPasses(frameContext);

	// submit passes
	swapchain->waitForImageInFlight(frameContext.imageIndex);
	swapchain->ResetFence();
	frameRenderer->submitPasses(frameContext);

	// present frame
    presentFrame();
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

    currentFrameIndex = (currentFrameIndex + 1) % Swap_chain::MAX_FRAMES_IN_FLIGHT;
}
