#pragma once

#include "../base/Swap_chain.h"
//#include "GlobalRenderSystem.h"

class BaseRenderPass
{
public:

	BaseRenderPass(Device& device_, AssetManager& assets_)
		: device(device_), assets(assets_) {}

	BaseRenderPass(const BaseRenderPass&) = delete;
	BaseRenderPass& operator=(const BaseRenderPass&) = delete;

	virtual ~BaseRenderPass() = default;


	virtual void createRenderSystems() = 0;
	/**
	* add a renderSytem for the pass with global descriptor set layouts
	*/
	//void addRenderSystem(std::unique_ptr<GlobalRenderSystem> newRenderSystem);

	/**
	* record pass in global command buffer
	*/
	virtual void recordPass() = 0;

	/**
	* get the render pass handle
	*/
	VkRenderPass getRenderPass() const {
		return renderPass;
	};

	/**
	 * update local reference to swapchain
	 */
	//virtual void updateSwapchain(Swap_chain& swapchain_) {};


private:
	virtual void createRenderPass() = 0;

	//std::vector<std::unique_ptr<GlobalRenderSystem>> renderSystems;
	VkRenderPass renderPass{ VK_NULL_HANDLE };

	Device& device;
	AssetManager& assets;
};