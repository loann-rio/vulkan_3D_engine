#include "PostProPass.h"

void PostProPass::recordPass(ObjectManager& objectManager, FrameInfo& frameInfo, VkCommandBuffer& commandBuffer)
{
    beginRenderPass(commandBuffer, 0, frameInfo.imageIndex);

    postProcessingRenderSystem->renderFullScreen(
        commandBuffer,
        {
            frameInfo.globalDescriptorSet[frameInfo.frameIndex],
            frameInfo.postProDescriptorSet[frameInfo.frameIndex]
        },
        glm::mat4(),
        glm::mat4()
    );

    endRenderPass(commandBuffer);
}

void PostProPass::createRenderPass(VkFormat imageFormat, VkFormat /**/)
{
    VkAttachmentDescription color{};
    color.format = imageFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

void PostProPass::beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex)
{
    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = renderPass;

    rpBegin.framebuffer = target->getFrameBuffer(frameIndex);
    
    rpBegin.renderArea.offset = { 0, 0 };
    rpBegin.renderArea.extent = target->getExtent();

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = { { 0.f, 1.f, 0.1f, 1.0f } };

    rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpBegin.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(target->getExtent().width);
    viewport.height = static_cast<float>(target->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{ {0, 0}, target->getExtent() };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void PostProPass::endRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

void PostProPass::createRenderSystems()
{
    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    RenderSystemBuilder postProBuilder{};
    postProBuilder.fragFilepath = "shaders\\PostProShader.frag.spv";
    postProBuilder.vertFilepath = "shaders\\fullscreen.vert.spv";
    postProBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout() };
    postProBuilder.renderPass = renderPass;
    postProBuilder.isFullscreenRender = true;
    postProcessingRenderSystem = GlobalRenderSystem::create<Model>(device, assets, postProBuilder);
}
