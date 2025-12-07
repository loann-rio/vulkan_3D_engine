#pragma once

///// https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/

#include "Device.h"
#include "Swap_chain.h"

#include "../assetManager/AssetManager.h"
#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

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

	DepthSwapChain(Device& deviceRef, AssetManager& assets, VkExtent2D depthImageExtent);
	~DepthSwapChain();

	DepthSwapChain(const DepthSwapChain&) = delete;
	DepthSwapChain& operator=(const DepthSwapChain&) = delete;

	VkFramebuffer getDepthFramebuffers(int index) { return depthFramebuffers[index]; }
	VkRenderPass getDepthRenderPass() const { return depthRenderPass; }
	
	VkImageView getDepthImageView(int index) { return assets.textures().get(textureTarget[index])->view(); }

	TextureManager::TextureID getTexture(int index) { return textureTarget[index]; }

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

	std::vector<TextureManager::TextureID> textureTarget;

	std::vector<std::array<VkDescriptorImageInfo, MAX_DEPTH_RENDER_COUNT>> descriptorImageInfo; 

	Device& device;
	AssetManager& assets;
};

