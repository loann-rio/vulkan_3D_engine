#include "RenderPassBase.h"

void PassCommandBuffers::allocate()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device.getThreadCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = buffers.size();

    if (vkAllocateCommandBuffers(
        device.device(),
        &allocInfo,
        buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("MainPass: failed to allocate command buffers");
    }
}

void PassTarget::resizeTargets(VkExtent2D newExtent)
{
    extent = newExtent;
    cleanupTargetTextures();
    createTargetTexture();

    cleanupLocalFramebuffers();
    createLocalFramebuffers();
}

void PassTarget::createLocalFramebuffers()
{
}

void PassTarget::cleanupLocalFramebuffers()
{
}

void PassTarget::cleanupTargetTextures()
{
}

void PassTarget::createTargetTexture()
{
}

void PassTarget::resizeTargets(VkExtent2D newExtent)
{
}
