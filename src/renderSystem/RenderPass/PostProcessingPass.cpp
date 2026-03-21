#include "PostProcessingPass.h"


PostProcessingPass::PostProcessingPass(
    Device& device,
    AssetManager& assets,
    Swapchain& swapchain,
    uint32_t framesInFlight,
    VkExtent2D extent,
    PassTarget* previousPassTarget,
    bool isFinal
)
    : RenderPassBase(device, assets),
    swapchain(&swapchain),
    extent(extent),
    inputColor(previousPassTarget)
{
    commandBuffers = std::make_unique<PassCommandBuffers>(device, framesInFlight);

    if (!isFinal) {
        passTarget = std::make_unique<PassTarget>(
            device, swapchain, assets, extent, VK_FORMAT_UNDEFINED, false
        );
    }
    else {
        setAsFinal();
    }

    createRenderPass();
    createPassDescriptorSetLayout();
}


void PostProcessingPass::createRenderPass()
{
    VkAttachmentDescription color{};
    color.format = swapchain->format();
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = isFinalPass
        ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &color;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dep;

    if (vkCreateRenderPass(device.device(), &info, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessingPass: render pass creation failed");
    }
}


void PostProcessingPass::createPassDescriptorSetLayout()
{
    setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT
        )
        .build();
}

void PostProcessingPass::bindGlobalDescriptorSet(VkCommandBuffer cmd, FrameContext& frameContext) const
{

    VkDescriptorSet set =
        inputColor->getColorDescriptorSet(frameContext.frameIndex);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderSystems[0]->getPipelineLayout(),
        0,
        1,
        &set,
        0,
        nullptr
	);
}

VkCommandBuffer PostProcessingPass::beginCommandBuffer(uint32_t frameIndex)
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

void PostProcessingPass::beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex)
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

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { { 0.f, 1.f, 0.1f, 1.0f } };

    rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpBegin.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void PostProcessingPass::setupViewportAndScissor(VkCommandBuffer cmd)
{
    VkExtent2D extent = swapchain->getExtent();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void PostProcessingPass::record(FrameContext& frame)
{
    VkCommandBuffer cmd = beginCommandBuffer(frame.frameIndex);

    beginRenderPass(cmd, frame.imageIndex);
    setupViewportAndScissor(cmd);
	bindGlobalDescriptorSet(cmd, frame);
    drawFullscreen(cmd);

    vkCmdEndRenderPass(cmd);
    endCommandBuffer(cmd);
}

void PostProcessingPass::drawFullscreen(VkCommandBuffer cmd)
{
    auto& system = renderSystems[0];
	FrameContext frameContext = FrameContext();
    system->record(cmd, frameContext, {});
}

void PostProcessingPass::endCommandBuffer(VkCommandBuffer cmd)
{
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessingPass: vkEndCommandBuffer failed");
	}
}

std::vector<VkCommandBuffer> PostProcessingPass::getFrameCommandBuffers(uint32_t frameIndex) const
{
    return { commandBuffers->get(frameIndex) };
}

void PostProcessingPass::createLocalFramebuffers()
{
    if (!isFinalPass) {
        passTarget->createLocalFramebuffers(renderPass);
    }
}

void PostProcessingPass::resizeTargets(VkExtent2D newExtent)
{
    extent = newExtent;
    if (!isFinalPass) {
        passTarget->resizeTargets(
            newExtent,
            swapchain->imageCount(),
            swapchain->format(),
            VK_FORMAT_UNDEFINED
        );
    }
}

void PostProcessingPass::updateSwapchain(Swapchain& swapchain_)
{
    swapchain = &swapchain_;
}