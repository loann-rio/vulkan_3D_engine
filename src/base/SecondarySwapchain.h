#pragma once

#include "Device.h"
#include "../objects/Texture.h"

// vulkan headers
#include <vulkan/vulkan.h>

struct SwapChainBuilder
{
	std::shared_ptr<Texture> depthTarget;
	std::shared_ptr<Texture> colorTarget;
	VkFormat depthFormat;
	VkFormat imageFormat;

	VkExtent2D windowExtent;

	VkRenderPass renderPass;

	std::vector<VkImageView> additionalImageView;
};


class SecondarySwapchain {
public:
	SecondarySwapchain(Device& device, SwapChainBuilder builder);
	~SecondarySwapchain();

	SecondarySwapchain(const SecondarySwapchain&) = delete;
	SecondarySwapchain& operator=(const SecondarySwapchain&) = delete;

	void submitCommandBuffer(const VkCommandBuffer* commandBuffer);

	VkFormat findDepthFormat();
	void createFramebuffer(VkExtent2D extent, VkFramebuffer& framebuffer, int indexFB = -1);

	VkRenderPass getRenderPass() const { return renderPass; }
	VkFramebuffer getFrameBuffer(int i = 0) const { return framebuffers[i]; }

private:
	void init();
	void createSwapChain();
	void createRenderPass();

	std::shared_ptr<Texture> depthTarget;
	std::shared_ptr<Texture> colorTarget;

	VkExtent2D extent;

	bool hasDepth = false;
	bool hasColor = false;

	VkRenderPass renderPass;
	bool ownsRenderPass = false;

	VkFormat depthFormat;
	VkFormat colorFormat;

	// if the image view is not the same from creation and render of a texture:
	// if defined, this will be the rendering target(s)
	std::vector<VkImageView> additionalImageView;

	// vector to hold per-face framebuffers
	std::vector<VkFramebuffer> framebuffers;

	// If we create per-face depth views, store them to destroy later:
	std::vector<VkImageView> perFaceDepthViews;

	// Whether we created the per-face color views or they were passed in
	bool ownsAdditionalViews = false;


	Device& device;

};