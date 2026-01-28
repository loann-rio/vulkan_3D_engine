#include "MainPass.h"

MainPass::MainPass(Device& device, AssetManager& assets, Swapchain& swapchain, DescriptorPool& renderPool, uint32_t frame_in_flight, VkExtent2D extent)
	: RenderPassBase(device, assets), swapchain( &swapchain ), extent( extent )
{ 
    commandBuffers = std::make_unique<PassCommandBuffers>(device, frame_in_flight);
	passTarget = std::make_unique<PassTarget>(device, swapchain, assets, extent);

    createRenderPass();

    createPassDescriptorSetLayout();
}

void MainPass::record(FrameContext& frame)
{
    const uint32_t frameSlot = frame.frameIndex;
    const uint32_t imageIndex = frame.imageIndex;

    VkCommandBuffer cmd = beginCommandBuffer(frameSlot);

	beginRenderPass(cmd, imageIndex);
    
	setupViewportAndScissor(cmd);

	bindGlobalDescriptorSet(cmd, frame);

	renderScene(cmd, frame);

    vkCmdEndRenderPass(cmd);

	endCommandBuffer(cmd);
}


VkCommandBuffer MainPass::getCommandBuffer(uint32_t frameIndex) const
{
    return commandBuffers->get(frameIndex);
}

void MainPass::createLocalFramebuffers()
{
	passTarget->createLocalFramebuffers(renderPass);
}

void MainPass::resizeTargets(VkExtent2D newExtent)
{
    extent = newExtent;
	passTarget->resizeTargets(
        newExtent, 
        swapchain->imageCount(),
        swapchain->format(), swapchain->depthFormat()
    );
}

void MainPass::createRenderPass()
{
    std::cout << "render path creation \n";
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = swapchain->depthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain->format();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void MainPass::createPassDescriptorSetLayout()
{
    /// global layout
    setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
}

void MainPass::bindGlobalDescriptorSet(VkCommandBuffer cmd, FrameContext& frameContext) const
{       
    uint32_t globalSetIndex = 0; // TODO: make configurable

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderSystems[0]->getPipelineLayout(),
        globalSetIndex,
        1,                         /*    descriptor count    */
        &frameContext.globalSet,   /*    pDescriptorSets     */
        0,                         /*  dynamic offset count  */
        nullptr                    /*    pDynamicOffsets     */
    );
}



void MainPass::updateSwapchain(Swapchain& swapchain_)
{
	this->swapchain = &swapchain_;
}

VkCommandBuffer MainPass::beginCommandBuffer(uint32_t frameIndex)
{
    VkCommandBuffer cmd = commandBuffers->get(frameIndex);

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: vkBeginCommandBuffer failed");
    }

    return cmd;
}

void MainPass::beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex)
{
    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = renderPass;

    if (isFinalPass) {
        rpBegin.framebuffer = swapchain->getFramebuffer(imageIndex);
    }
    else {
        rpBegin.framebuffer = passTarget->framebuffers[imageIndex];
    }

    rpBegin.renderArea.offset = { 0, 0 };
    rpBegin.renderArea.extent = swapchain->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 1.f, 0.05f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpBegin.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void MainPass::setupViewportAndScissor(VkCommandBuffer cmd)
{
    VkExtent2D extent = swapchain->getExtent();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void MainPass::renderScene(VkCommandBuffer cmd, FrameContext& frame)
{
    for (auto& system : renderSystems) {

        frame.renderItems.clear();

        for (GameObjectModel* obj : frame.listGameObjects)
        {
            if (!obj || !obj->show || obj->toBeRemoved)
                continue;

            if (!obj->modelAsset)
                continue;

            if (!system->accepts(*obj))
                continue;

            RenderItem item{};
            item.model = assets.models().get(obj->modelAsset);
            item.modelMatrix = obj->getTransformMat();
            item.normalMatrix = obj->getNormalMat();

            frame.renderItems.push_back(item);
        }

        if (!frame.renderItems.empty())
            system->record(cmd, frame, frame.renderItems);
    }
}

void MainPass::endCommandBuffer(VkCommandBuffer cmd)
{
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: vkEndCommandBuffer failed");
    }
}
