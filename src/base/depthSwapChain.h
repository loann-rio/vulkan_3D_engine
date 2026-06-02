#pragma once

///// https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/

#include "Device.h"

class DepthSwapChain
{
public:
	static constexpr int MAX_DEPTH_RENDER_COUNT = 4; 

	DepthSwapChain(Device& deviceRef, VkExtent2D depthImageExtent);
	~DepthSwapChain();

	DepthSwapChain(const DepthSwapChain&) = delete;
	DepthSwapChain& operator=(const DepthSwapChain&) = delete;

	VkRenderPass getDepthRenderPass() const { return depthRenderPass; }
	VkExtent2D getDepthSwapChainExtent() { return depthExtent; }

	VkFormat findDepthFormat();

private:
	void createDepthRenderPass();

	VkFormat swapChainDepthFormat;
	VkExtent2D depthExtent;
	VkRenderPass depthRenderPass;

	Device& device;
};

