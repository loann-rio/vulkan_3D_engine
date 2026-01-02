#include "SwapChain.h"

Swapchain::Swapchain(Device& device, AssetManager& assets, VkExtent2D extent, std::shared_ptr<Swapchain> oldSwapchain)
{
}

Swapchain::~Swapchain()
{
}

bool Swapchain::acquireNextImage(uint32_t& imageIndex)
{
	return false;
}

void Swapchain::present(uint32_t imageIndex)
{
}

void Swapchain::advanceFrame()
{
}

void Swapchain::createSwapchain()
{
}

void Swapchain::createImageViews()
{
}

void Swapchain::createRenderPass()
{
}

void Swapchain::createFramebuffers()
{
}

void Swapchain::createSyncObjects()
{
}
