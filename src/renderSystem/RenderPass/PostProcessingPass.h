#pragma once

#include "RenderPassBase.h"
#include "../FrameContext.h"
#include "../SwapChain.h"

#include <memory>
#include <vector>

class PostProcessingPass final : public RenderPassBase {
public:
    PostProcessingPass(
        Device& device,
        AssetManager& assets,
        Swapchain& swapchain,
        uint32_t framesInFlight,
        VkExtent2D extent,
        PassTarget* previousPassTarget,
        bool isFinal = true
    );

    void record(FrameContext& frame) override;
    std::vector<VkCommandBuffer> getFrameCommandBuffers(uint32_t frameIndex) const override;

    void createLocalFramebuffers() override;
    void resizeTargets(VkExtent2D newExtent) override;
    void updateSwapchain(Swapchain& swapchain_) override;

private:
    void createRenderPass() override;
    void createPassDescriptorSetLayout() override;

	void bindGlobalDescriptorSet(VkCommandBuffer cmd, FrameContext& frameContext) const override;

    VkCommandBuffer beginCommandBuffer(uint32_t frameIndex);
    void beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex);
    void setupViewportAndScissor(VkCommandBuffer cmd);
    void drawFullscreen(VkCommandBuffer cmd);
    void endCommandBuffer(VkCommandBuffer cmd);

private:
    std::unique_ptr<PassCommandBuffers> commandBuffers;
    std::unique_ptr<PassTarget> passTarget;

    Swapchain* swapchain;
    VkExtent2D extent;

	PassTarget* inputColor;
    
};
