#include "GlobalRenderer.h"

#include "Swapchain.h"
#include "FrameContext.h"


#include "../base/Device.h"
#include "RenderPass/MainPass.h"
#include "RenderPass/ShadowPass.h"
#include "RenderPass/PostProcessingPass.h"

#include "RenderSystems/RenderSystemBuilder.h"
#include "../model/Vertex/ObjVertexData.h"
 
#include <stdexcept>
#include <cassert>

GlobalRenderer::GlobalRenderer(Device& device, Window& window, AssetManager& assetManager, ObjectManager& objectManager)
	: device(device), window(window), assetManager(assetManager), objectManager(objectManager)
{
    recreateSwapchain();
    createUboDescriptorPool();
    createFrameRenderer();
    createGlobalUniformBuffer();

    createFrameContext();


    //imgui = std::make_unique<BasicUI>(device, assetManager, window.getGLFWwindow(), globalRenderPass);
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

        oldSwapChain->destroySwapchainOnly();
    }

    frameContext.swapchain = swapchain.get();

    if (frameRenderer) {
        for (size_t i = 0; i < frameRenderer->getPassCount() - 1; ++i) {
            auto& pass = frameRenderer->getPass(i);
			pass.updateSwapchain(*swapchain);
            pass.createLocalFramebuffers();
        }

        // final pass uses swapchain framebuffers
        auto& finalPass = frameRenderer->getLastPass();
        finalPass.updateSwapchain(*swapchain);
        swapchain->createFramebuffers(finalPass.getRenderPass());
        finalPass.setAsFinal();
    }
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
            .cullMode(VK_CULL_MODE_NONE)
            .renderPass(mainPass->getRenderPass())
            .vertexLayout(new ObjVertexLayout)
            .asMainRenderSystem();

        mainPass->addRenderSystem(baseRenderSystem);
    }

    frameRenderer->addPass(std::move(mainPass));

    auto* targetMainPass = frameRenderer->getLastPass().getTarget();

    auto postProcessPass = std::make_unique<PostProcessingPass>(
        device, assetManager,
        *swapchain,
        Swapchain::MAX_FRAMES_IN_FLIGHT,
        swapchain->getExtent(), 
        targetMainPass,
        true
    );

    /*auto shadowPass = std::make_unique<ShadowPass>(
		device, assetManager,
        *swapchain, *globalPool,
		Swapchain::MAX_FRAMES_IN_FLIGHT,
        swapchain->getExtent()

    );

   {
        auto shadowRenderSystem = RenderSystemBuilder()
            .vertexShader("shaders/ShadowPass.vert.spv")
            .cullMode(VK_CULL_MODE_NONE)
            .renderPass(shadowPass->getRenderPass())
            .vertexLayout(new ObjVertexLayout)
            .asShadowRenderSystem();
        shadowPass->addRenderSystem(shadowRenderSystem);
    }*/


    createFrameBuffers();
}

void GlobalRenderer::createFrameBuffers()
{
    for (size_t i = 0; i < frameRenderer->getPassCount() - 1; ++i) {
		auto& pass = frameRenderer->getPass(i);
		pass.createLocalFramebuffers();
    }

	// final pass uses swapchain framebuffers
    globalRenderPass = frameRenderer->getLastPass().getRenderPass();
	swapchain->createFramebuffers(globalRenderPass);
    frameRenderer->getLastPass().setAsFinal();
}

void GlobalRenderer::createGlobalUniformBuffer()
{
    uboBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++)
    {
        uboBuffers[i] = std::make_unique<Buffer>(
            device,
            sizeof(GlobalUbo_),
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

    globalDescriptorSet.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size() && i < 2; i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]);
    }
}

bool GlobalRenderer::aquireNextImage()
{
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


void GlobalRenderer::renderFrame(std::vector<GameObjectModel*> listGameObjects)
{
	// acquire next image
    if (!aquireNextImage()) {
        return;
    }

	// setup frame context
    frameContext.frameIndex = static_cast<uint32_t>(currentFrameIndex);
    frameContext.imageIndex = static_cast<uint32_t>(currentImageIndex);
	frameContext.listGameObjects = listGameObjects;
	frameContext.globalSet = globalDescriptorSet[frameContext.frameIndex];
   

	// update global UBO
	updateGlobalUniformBuffer(frameContext.frameIndex);

	// record passes 
    frameRenderer->recordPasses(frameContext);

	// submit passes
	swapchain->waitForImageInFlight(frameContext.imageIndex);

	// reset fence
    VkFence fenceToReset = swapchain->getInFlightFence(frameContext.frameIndex);
    vkResetFences(device.device(), 1, &fenceToReset);

    // submit passes
	frameRenderer->submitPasses(frameContext);

	// present frame
    presentFrame();
}

void GlobalRenderer::presentFrame()
{
    VkResult result = swapchain->present(frameContext.imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        window.wasWindowResized()) {

        window.resetWindowResizedFlag();
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present frame");
    }

    currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void GlobalRenderer::updateGlobalUniformBuffer(uint32_t frameIndex)
{
    uboBuffers[frameIndex]->writeToBuffer(&ubo);
	uboBuffers[frameIndex]->flush();
}

void GlobalRenderer::createFrameContext()
{
    frameContext.swapchain = swapchain.get();

    createTimelineSemaphore();
}

void GlobalRenderer::createTimelineSemaphore()
{
    VkSemaphoreTypeCreateInfo typeInfo{};
    typeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    typeInfo.initialValue = 1;

    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = &typeInfo;


    VkResult result = vkCreateSemaphore(device.device(), &info, nullptr, &frameContext.timelineSemaphore);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create timeline semaphore");
	}
}


