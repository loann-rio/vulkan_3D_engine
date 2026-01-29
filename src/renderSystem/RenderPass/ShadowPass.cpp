#include "ShadowPass.h"

ShadowPass::ShadowPass(Device& device, AssetManager& assets, Swapchain& swapchain, DescriptorPool& renderPool, uint32_t frame_in_flight, VkExtent2D extent)
	: RenderPassBase(device, assets), swapchain(&swapchain), extent(extent)
{ 
	commandBuffers = std::make_unique<PassCommandBuffers>(device, frame_in_flight * MAX_DEPTH_RENDER_COUNT);
	
    for (uint32_t i = 0; i < MAX_DEPTH_RENDER_COUNT; i++)
		passTarget.emplace_back(std::make_unique<PassTarget>(device, swapchain, assets, extent));

	createRenderPass();
	createPassDescriptorSetLayout();
}

void ShadowPass::record(FrameContext& frame)
{
	const uint32_t frameSlot = frame.frameIndex;
	const uint32_t imageIndex = frame.imageIndex;


	for (size_t i = 0; i < MAX_DEPTH_RENDER_COUNT; i++) {
        VkCommandBuffer cmd = beginCommandBuffer(frameSlot * MAX_DEPTH_RENDER_COUNT + i);

        beginRenderPass(cmd, frame.imageIndex, i);

        setupViewportAndScissor(cmd);

        bindGlobalDescriptorSet(cmd, frame);

        renderScene(cmd, frame);

        vkCmdEndRenderPass(cmd);

        endCommandBuffer(cmd);
    }
}

VkCommandBuffer ShadowPass::getCommandBuffer(uint32_t frameIndex) const
{
	return commandBuffers->get(frameIndex);
}

void ShadowPass::createLocalFramebuffers()
{
	for (auto& passTarget_ : passTarget)
	    passTarget_->createLocalFramebuffers(renderPass);
}

void ShadowPass::resizeTargets(VkExtent2D newExtent)
{
	extent = newExtent;
	for (auto& passTarget_ : passTarget)
        passTarget_->resizeTargets(
		    newExtent,
		    swapchain->imageCount(),
		    swapchain->format(), swapchain->depthFormat()
	    );
}

void ShadowPass::updateSwapchain(Swapchain& swapchain_)
{
    this->swapchain = &swapchain_;
}

void ShadowPass::createRenderPass()
{
    std::cout << "depth render path creation \n";
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = swapchain->depthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store depth results
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = 0;  // The first subpass
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // Fragment shader reads the image
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;  // Depth attachment write
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;  // The image is being read in the fragment shader 
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // Depth attachment write
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pre-render depth pass!");
    }
}

void ShadowPass::createPassDescriptorSetLayout()
{
    /// global layout
    setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();
}

void ShadowPass::bindGlobalDescriptorSet(VkCommandBuffer cmd, FrameContext& frameContext) const
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

VkCommandBuffer ShadowPass::beginCommandBuffer(uint32_t frameIndex)
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

void ShadowPass::beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t targetIndex)
{
    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = renderPass;

    rpBegin.framebuffer = passTarget[targetIndex]->framebuffers[imageIndex];

    rpBegin.renderArea.offset = { 0, 0 };
    rpBegin.renderArea.extent = swapchain->getExtent();

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = { 1.0f, 0 };

    rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpBegin.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void ShadowPass::setupViewportAndScissor(VkCommandBuffer cmd)
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

void ShadowPass::renderScene(VkCommandBuffer cmd, FrameContext& frame)
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

void ShadowPass::endCommandBuffer(VkCommandBuffer cmd)
{
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: vkEndCommandBuffer failed");
    }
}
