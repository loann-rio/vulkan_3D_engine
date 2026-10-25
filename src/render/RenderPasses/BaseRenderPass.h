#pragma once

#include "../../base/Swap_chain.h"
#include "../PassTarget.h"
#include "../../objects/objectManager.h"
#include "../../base/Frame_info.h"
#include "../../objects/BasicUI.h"

class BaseRenderPass
{
public:

	BaseRenderPass(Device& device_, AssetManager& assets_)
		: device(device_), assets(assets_) {}

	BaseRenderPass(const BaseRenderPass&) = delete;
	BaseRenderPass& operator=(const BaseRenderPass&) = delete;

	virtual ~BaseRenderPass() {
		if (renderPass != nullptr) {
			vkDestroyRenderPass(device.device(), renderPass, nullptr);
		};
	}

	virtual void createRenderSystems() = 0;
	
	/**
	record pass in global command buffer
	*/
	virtual void recordPass(
		ObjectManager& objectManager,
		FrameInfo& frameInfo,
		VkCommandBuffer& commandBuffer
	) = 0;

	/**
	get the render pass handle
	*/
	VkRenderPass getRenderPass() const {
		return renderPass;
	};

	/* 
	begin render pass
	*/
	virtual void beginRenderPass(VkCommandBuffer commandBuffer, int depthRenderIndex, int frameIndex) = 0;

	/*
	end render pass
	*/
	virtual void endRenderPass(VkCommandBuffer commandBuffer) = 0;

	/*
	set target of the pass
	*/
	void setTarget(PassTarget* target_) { target = target_; }

	/*
	set ui
	*/
	/*void setUi(
		BasicUI* ui
	) {};*/
	


protected:
	virtual void createRenderPass(VkFormat imageFormat, VkFormat depthFormat) = 0;

	//std::vector<std::unique_ptr<GlobalRenderSystem>> renderSystems;

	VkRenderPass renderPass{ VK_NULL_HANDLE };

	PassTarget* target = nullptr;

	Device& device;
	AssetManager& assets;
};