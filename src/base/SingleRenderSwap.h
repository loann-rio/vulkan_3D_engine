#pragma once

#include "Device.h"
#include "../objects/Texture.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>
#include <array>

class SingleSwapChain
{
public:
	SingleSwapChain(Device& deviceRef, VkExtent2D imageExtent);
	~SingleSwapChain();

	SingleSwapChain(const SingleSwapChain&) = delete;
	SingleSwapChain& operator=(const SingleSwapChain&) = delete;

	VkRenderPass getRenderPass() const { return renderPass; }
	
	VkImageView getColorImageView() { return textureTargetColor->getImageView(); }
	VkImageView getDepthImageView() { return textureTargetDepth->getImageView(); }

	VkFramebuffer getFrameBuffer() const { return frameBuffer; }
	
	std::shared_ptr<Texture> getTextureColor() { return textureTargetColor; }
	std::shared_ptr<Texture> getTextureDepth() { return textureTargetDepth; }

	VkExtent2D getSwapChainExtent() { return textureExtent; }

	uint32_t width() const { return textureExtent.width; }
	uint32_t height() const { return textureExtent.height; }

	VkFormat findDepthFormat();

	VkDescriptorImageInfo getDepthImageInfo() { return depthDescriptorImageInfo; }
	VkDescriptorImageInfo getColorImageInfo() { return colorDescriptorImageInfo; }

	void submitCommandBuffers(const VkCommandBuffer& buffers);

private:
	void init();

	void createDepthResources();
	void createColorResources();
	void createRenderPass();
	void createFrameBuffers();

	VkFormat swapChainDepthFormat;
		
	VkExtent2D textureExtent;

	VkFramebuffer frameBuffer;

	VkRenderPass renderPass;

	std::shared_ptr<Texture>  textureTargetColor;
	std::shared_ptr<Texture>  textureTargetDepth;

	VkDescriptorImageInfo depthDescriptorImageInfo;
	VkDescriptorImageInfo colorDescriptorImageInfo;

	Device& device;
};

