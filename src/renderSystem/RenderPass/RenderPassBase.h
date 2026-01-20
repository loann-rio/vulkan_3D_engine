#pragma once

#include "../FrameContext.h"
#include "../../base/Device.h"

#include "../RenderSystems/BaseRenderSystem.h"

#include "../../assetManager/AssetManager.h"
#include "../RenderSystems/RenderSystemBuilder.h"

#include <vulkan/vulkan_core.h>
#include <vector>
#include <memory>

class RenderPassBase {
public:
    
    virtual ~RenderPassBase() = default; 

	RenderPassBase(Device& device_, AssetManager& assets_)
        : device(device_), assets(assets_) {}

    RenderPassBase(const RenderPassBase&) = delete;
    RenderPassBase& operator=(const RenderPassBase&) = delete;

    /**
     * record commands for this pass into command buffer
     */
    virtual void record(FrameContext& frame) = 0;

    /**
     * returns the command buffer for a given frame index
     */
    virtual VkCommandBuffer commandBuffer(uint32_t frameIndex) const = 0;

    /**
     * create the render pass
	 */
	virtual void createRenderPass() = 0;

    /**
     * create the global descriptor sets for the pass
	 */
	virtual void createPassDescriptorSetLayout() = 0;

    /**
	 * add a renderSytem for the pass with global descriptor set layouts
     */
    void addRenderSystem(
        RenderSystemBuilder system
    ) {
		system.setGlobalSetLayout(setLayout.get());
        renderSystems.emplace_back(system.build(device, assets));
    };

    /**
	 * get the render pass handle
     */
    VkRenderPass getRenderPass() const {
        return renderPass;
    }

	/**
     * create local framebuffers for the pass
	 */
	virtual void createLocalFramebuffers() {}

    /**
	 * set as final pass with swapchain framebuffers
     */
    virtual void setAsFinal() { isFinalPass = true; }

protected:

    virtual void bindGlobalDescriptorSet(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const {}

    RenderPassBase() = default;

    bool isFinalPass = false;

    // One primary command buffer per frame in flight and per target
    std::vector<VkCommandBuffer> commandBuffers;

    // Render systems used by this pass
    std::vector<std::unique_ptr<BaseRenderSystem>> renderSystems;

    // Timeline semaphore value signaled by this pass
    uint64_t signaledTimelineValue{ 0 };

    VkRenderPass renderPass{ VK_NULL_HANDLE };  

    std::unique_ptr<DescriptorSetLayout> setLayout;

    Device& device;

    AssetManager& assets;
};
