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
     * submit commands for this pass
     */
    virtual void submit(FrameContext& frame) = 0;

    /**
     * Returns the command buffer for a given frame index
     */
    virtual VkCommandBuffer commandBuffer(uint32_t frameIndex) const = 0;

    /**
     * Called by FrameRenderer before submission to assign
     * timeline semaphore value this pass will signal
     */
    virtual void setTimelineValue(uint64_t value) = 0;

    /**
     * Returns the last timeline value signaled by this pass
     */
    virtual uint64_t timelineValue() const = 0;

    /**
     * Add a renderSytem for the pass
     */
    void addRenderSystem(
        std::unique_ptr<BaseRenderSystem> system
    ) {
        renderSystems.emplace_back(std::move(system));
    };

protected:
    RenderPassBase() = default;

    // One primary command buffer per frame in flight and per target
    std::vector<VkCommandBuffer> commandBuffers;

    // Render systems used by this pass
    std::vector<std::unique_ptr<BaseRenderSystem>> renderSystems;

    // Timeline semaphore value signaled by this pass
    uint64_t signaledTimelineValue{ 0 };

};
