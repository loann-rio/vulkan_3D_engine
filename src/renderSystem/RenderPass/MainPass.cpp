#include "MainPass.h"

MainPass::MainPass(Device& device, AssetManager& assets, Swapchain& swapchain, uint32_t frame_in_flight, VkExtent2D extent)
	: device( device ), swapchain( swapchain ), assets( assets ), extent( extent )
{ 
	allocateCommandBuffers(frame_in_flight);
    createTargetTexture();
	createFramebuffers();
}

MainPass::~MainPass()
{
	for (VkFramebuffer fb : framebuffers) {
		vkDestroyFramebuffer(device.device(), fb, nullptr);
	}
}

void MainPass::record(FrameContext& frame)
{
    const uint32_t frameIndex = frame.frameIndex;
    const uint32_t imageIndex = frame.imageIndex;

    VkCommandBuffer commandBuffer = beginPass(frameIndex);

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = renderPass;
    rpBegin.framebuffer = framebuffers[imageIndex];
    rpBegin.renderArea.offset = { 0, 0 };
    rpBegin.renderArea.extent = swapchain.getExtent();

    VkClearValue clear{};
    clear.color = { {0.05f, 0.05f, 0.1f, 1.0f} };
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clear;

    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    for (auto& system : renderSystems) {
        system->record(commandBuffer, frame);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: vkEndCommandBuffer failed");
    }
}

void MainPass::submit(FrameContext& frame)
{
    // Submit command buffer and signal timeline semaphore
    VkCommandBuffer cmd = commandBuffers[frame.frameIndex];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    // Timeline semaphore signal
    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    uint64_t signalValue = frame.timelineValue;
    timelineInfo.signalSemaphoreValueCount = 1;
    timelineInfo.pSignalSemaphoreValues = &signalValue;

    submitInfo.pNext = &timelineInfo;

    VkSemaphore signalSemaphores[] = { frame.timeline };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset and submit using Device helper (provides queue synchronization)
    vkResetFences(device.device(), 1, &frame.inFlightFence);
    device.submitToGraphicQueue(submitInfo, frame.inFlightFence);

    // store last signaled value for this pass
    signaledTimelineValue = signalValue;
}

VkCommandBuffer MainPass::commandBuffer(uint32_t frameIndex) const
{
    return commandBuffers[frameIndex];
}

void MainPass::setTimelineValue(uint64_t value)
{
    signaledTimelineValue = value;
}

uint64_t MainPass::timelineValue() const
{
    return signaledTimelineValue;
}

void MainPass::allocateCommandBuffers(uint32_t framesInFlight)
{
    commandBuffers.resize(framesInFlight);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device.getThreadCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = framesInFlight;

    if (vkAllocateCommandBuffers(
        device.device(),
        &allocInfo,
        commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: failed to allocate command buffers");
    }
}

void MainPass::createFramebuffers()
{
    size_t imageCount = swapchain.imageCount();

    framebuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < framebuffers.size(); i++) 
    {
        std::array<VkImageView, 2> attachments = { 
            assets.textures().get(textureTarget[imageCount * i])->view(),
            assets.textures().get(textureTarget[imageCount * i + 1])->view()
        };

        VkExtent2D swapChainExtent = swapchain.getExtent();
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            device.device(),
            &framebufferInfo,
            nullptr,
            &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void MainPass::createTargetTexture()
{
    size_t imageCount = swapchain.imageCount();

    VkFormat format = swapchain.format();
    VkFormat depthFormat = swapchain.depthFormat();

    VkExtent2D swapChainExtent = swapchain.getExtent();

    // textures include both depth and color in alternative order
    textureTarget.resize(imageCount * 2);

    for (int i = 0; i < imageCount; i++)
    {
      
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 100.0f;

        TextureBuilder builder(device);
        textureTarget[imageCount * i] = assets.textures().create(
            builder.fromTextureInfo(
                swapchain.getImage(i), 
                { swapChainExtent.width, swapChainExtent.height },
                viewInfo, 
                samplerInfo, 
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                1, format)
        );
    }

    for (int i = 0; i < imageCount; i++)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;
        imageInfo.pNext = NULL;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 100.0f;

        TextureBuilder builder(device);
        textureTarget[imageCount * i + 1] = assets.textures().create(builder.fromTextureInfo(imageInfo, viewInfo, samplerInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }
}

VkCommandBuffer MainPass::beginPass(uint32_t frameIndex)
{
    VkCommandBuffer cmd = commandBuffers[frameIndex];

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: vkBeginCommandBuffer failed");
    }

    return cmd;
}
