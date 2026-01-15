#pragma once

#include "../FrameContext.h"
#include "../../base/Device.h"

#include "../RenderSystems/BaseRenderSystem.h"

#include "../../assetManager/AssetManager.h"

#include <vulkan/vulkan_core.h>
#include <vector>
#include <memory>

class RenderPassBase {
public:
    
    virtual ~RenderPassBase() = default; 

    RenderPassBase(const RenderPassBase&) = delete;
    RenderPassBase& operator=(const RenderPassBase&) = delete;

    /**
     * Record commands for this pass into command buffer
     */
    virtual void record(FrameContext& frame) = 0;

    /**
     * Returns the command buffer for a given frame index
     */
    virtual VkCommandBuffer commandBuffer(uint32_t frameIndex) const = 0;

    /**
     * Create the render pass
	 */
	virtual void createRenderPass() = 0;

    /**
     * Add a renderSytem for the pass
     */
    void addRenderSystem(
        std::unique_ptr<BaseRenderSystem> system
    ) {
        renderSystems.emplace_back(std::move(system));
    };

    /**
	 * Get the render pass handle
     */
    VkRenderPass getRenderPass() const {
        return renderPass;
    }

protected:
    RenderPassBase() = default;

    // One primary command buffer per frame in flight and per target
    std::vector<VkCommandBuffer> commandBuffers;

    // Render systems used by this pass
    std::vector<std::unique_ptr<BaseRenderSystem>> renderSystems;

    // Timeline semaphore value signaled by this pass
    uint64_t signaledTimelineValue{ 0 };

    VkRenderPass renderPass{ VK_NULL_HANDLE };  

};
