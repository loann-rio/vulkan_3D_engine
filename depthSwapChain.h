#pragma once

///// https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/

#include "Device.h"
#include "Swap_chain.h"


// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>
#include <array>

class DepthSwapChain
{
public:
	static constexpr int MAX_DEPTH_RENDER_COUNT = 4; 

	DepthSwapChain(Device& deviceRef, VkExtent2D depthImageExtent);
	~DepthSwapChain();

	DepthSwapChain(const DepthSwapChain&) = delete;
	DepthSwapChain& operator=(const DepthSwapChain&) = delete;

	VkFramebuffer getDepthFramebuffers(int index) { return depthFramebuffers[index]; }
	VkRenderPass getDepthRenderPass() const { return depthRenderPass; }
	VkImageView getDepthImageView(int index) { return depthImageViews[index]; }

	VkExtent2D getDepthSwapChainExtent() { return depthExtent; }

	uint32_t width() const { return depthExtent.width; }
	uint32_t height() const { return depthExtent.height; }

	VkFormat findDepthFormat();
	VkDescriptorImageInfo* getShadowImageInfo(int i) { return descriptorImageInfo[i].data(); }

private:
	void init();

	void createDepthResources();
	void createDepthRenderPass();
	void createDepthbuffers();
	void createDepthImageInfo();

	VkFormat swapChainDepthFormat;

	VkExtent2D depthExtent;

	std::vector<VkFramebuffer> depthFramebuffers;
	VkRenderPass depthRenderPass;

	std::vector<VkImage> depthImages;
	std::vector<VkDeviceMemory> depthImageMemorys;
	std::vector<VkImageView> depthImageViews;
	std::vector<VkSampler> depthSampler; 

	std::vector<std::array<VkDescriptorImageInfo, MAX_DEPTH_RENDER_COUNT>> descriptorImageInfo; 

	Device& device;
};

