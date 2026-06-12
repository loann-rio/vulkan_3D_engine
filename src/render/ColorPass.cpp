#include "ColorPass.h"

//#include "DepthPass.h"

void ColorPass::createRenderSystems()
{
    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    auto terrainSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    auto shadowSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4)// DepthPass::MAX_DEPTH_RENDER_COUNT)
        .build();

    auto skyboxSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    {
        RenderSystemBuilder gltfBuilder{};
        gltfBuilder.fragFilepath = "shaders\\GlTFshader.frag.spv";
        gltfBuilder.vertFilepath = "shaders\\GlTFshader.vert.spv";
        gltfBuilder.globalSetLayout = {
            globalSetLayout->getDescriptorSetLayout(),
            shadowSetLayout->getDescriptorSetLayout(),
            skyboxSetLayout->getDescriptorSetLayout()
        };
        gltfBuilder.renderPass = renderPass;
        gltfBuilder.hasMultipleInstance = true;

        gltfRenderSystem = GlobalRenderSystem::create<GlTFModel::ModelGltf>(device, assets, gltfBuilder);
    }

    {
        RenderSystemBuilder objBuilder{};
        objBuilder.fragFilepath = "shaders\\simple_shader.frag.spv";
        objBuilder.vertFilepath = "shaders\\simple_shader.vert.spv";
        objBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout(), shadowSetLayout->getDescriptorSetLayout() };
        objBuilder.renderPass = renderPass;
        objBuilder.hasMultipleInstance = true;

        objRenderSystem = GlobalRenderSystem::create<Model>(device, assets, objBuilder);
    }

    {
        RenderSystemBuilder skyboxBuilder{};
        skyboxBuilder.fragFilepath = "shaders\\skybox.frag.spv";
        skyboxBuilder.vertFilepath = "shaders\\skybox.vert.spv";
        skyboxBuilder.globalSetLayout = { globalSetLayout->getDescriptorSetLayout() };
        skyboxBuilder.renderPass = renderPass;
        skyboxBuilder.subModelType = ModelSubType::SKYBOX;
        skyboxBuilder.isSkyBox = true;
        skyboxRenderSystem = GlobalRenderSystem::create<Model>(device, assets, skyboxBuilder);
    }
}

void ColorPass::recordPass(ObjectManager& objectManager, FrameInfo& frameInfo, VkCommandBuffer& commandBuffer)
{
    beginRenderPass(commandBuffer, 0, frameInfo.imageIndex);

    if (objectManager.baseSkyBox)
        gltfRenderSystem->renderGameObjects(commandBuffer, frameInfo,
            {
                frameInfo.globalDescriptorSet[frameInfo.frameIndex],
                frameInfo.shadowDescriptorSet[frameInfo.frameIndex],
                assets.models().get(objectManager.baseSkyBox->modelAsset)->lods[0].materials[0].descriptorSet[frameInfo.frameIndex]
            },
            frameInfo.mainCameraFrustrumPlanes);
    else
        objectManager.baseSkyBox = dynamic_cast<GameObjectModel*>(objectManager.get("cubemap1"));


    objRenderSystem->renderGameObjects(
        commandBuffer,
        frameInfo,
        {
            frameInfo.globalDescriptorSet[frameInfo.frameIndex],
            frameInfo.shadowDescriptorSet[frameInfo.frameIndex]
        }
    );

    skyboxRenderSystem->renderGameObjects(
        commandBuffer,
        frameInfo,
        {
            frameInfo.globalDescriptorSet[frameInfo.frameIndex]
        }
    );

    imgui->drawUI(commandBuffer, &objectManager, frameInfo.gpuFrameRate);

    endRenderPass(commandBuffer);
}

void ColorPass::createRenderPass(VkFormat imageFormat, VkFormat depthFormat)
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
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
    colorAttachment.format = imageFormat;
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

void ColorPass::beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = target->getFrameBuffer(frameIndex);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = target->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.23f, 0.5f, 0.92f, 1.f };
    clearValues[1].depthStencil = { 1.0f, 0 };

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

void ColorPass::endRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}