#include "DepthPass.h"

#include "../../base/Device.h"

void DepthPass::createRenderSystems()
{
    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    auto shadowSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, DepthPass::MAX_DEPTH_RENDER_COUNT)
        .build();

    {
        RenderSystemBuilder gltfShadowBuilder{};
        gltfShadowBuilder.vertFilepath = "shaders\\shadowmapgltf.vert.spv";
        gltfShadowBuilder.globalSetLayout = { 
            globalSetLayout->getDescriptorSetLayout(), 
            shadowSetLayout->getDescriptorSetLayout() };
        gltfShadowBuilder.renderPass = renderPass;
        gltfShadowBuilder.hasMultipleInstance = true;

        depthRenderSystemGltf = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, assets, gltfShadowBuilder);
    }

    {
        RenderSystemBuilder objShadowBuilder{};
        objShadowBuilder.vertFilepath = "shaders\\shadowmap.vert.spv";
        objShadowBuilder.globalSetLayout = { 
            globalSetLayout->getDescriptorSetLayout(), 
            shadowSetLayout->getDescriptorSetLayout() };
        objShadowBuilder.renderPass = renderPass;
        objShadowBuilder.hasMultipleInstance = true;

        depthRenderSystem = GlobalRenderSystem::create<Model>(device, assets, objShadowBuilder);
    }

    {
        RenderSystemBuilder terrainShadowBuilder{};
        terrainShadowBuilder.vertFilepath = "shaders\\shadowMapTerrain.vert.spv";
        terrainShadowBuilder.globalSetLayout = { 
            globalSetLayout->getDescriptorSetLayout(), 
            shadowSetLayout->getDescriptorSetLayout() };
        terrainShadowBuilder.renderPass = renderPass;
        terrainShadowBuilder.hasMultipleInstance = true;
        terrainShadowBuilder.subModelType = ModelSubType::TERRAIN;

        depthTerrainRenderSystem = GlobalRenderSystem::create<Model>(device, assets, terrainShadowBuilder);
    }
}

void DepthPass::recordPass(ObjectManager & objectManager, FrameInfo & frameInfo, VkCommandBuffer & commandBuffer)
{
    for (int depthRenderIndex = 0; depthRenderIndex < DepthPass::MAX_DEPTH_RENDER_COUNT && depthRenderIndex < frameInfo.spotLightCount; depthRenderIndex++)
    {
        beginRenderPass(commandBuffer, depthRenderIndex, frameInfo.frameIndex);

        for (auto renderSystem : {
                depthRenderSystem,
                depthRenderSystemGltf,
                depthTerrainRenderSystem
            })

            renderSystem->renderGameObjectsDepth(
                commandBuffer,
                frameInfo,
                {
                    frameInfo.globalDescriptorSet[frameInfo.frameIndex],
                    frameInfo.shadowDescriptorSet[frameInfo.frameIndex]
                },
                depthRenderIndex,
                frameInfo.frameIndex
            );

        endRenderPass(commandBuffer);
    }
}

void DepthPass::createRenderPass(VkFormat imageFormat, VkFormat depthFormat)
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
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

void DepthPass::beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = target->getFrameBuffer(depthRenderIndex + frameIndex * MAX_DEPTH_RENDER_COUNT);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = target->getExtent();

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

void DepthPass::endRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}
