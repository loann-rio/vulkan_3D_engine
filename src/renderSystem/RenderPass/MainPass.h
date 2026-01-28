#pragma once

#include "RenderPassBase.h"
#include "../../base/Device.h"
#include "../FrameContext.h"
#include "../SwapChain.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>


class RenderSystem;

enum class PassSet : uint32_t {
    Frame = 0,
    System = 1,
    Material = 2
};

class MainPass final : public RenderPassBase {
public:
    MainPass(
        Device& device,
        AssetManager& assets,
        Swapchain& swapchain,
        DescriptorPool& renderPool,
        uint32_t frame_in_flight, 
        VkExtent2D extent
    );

    ~MainPass() override {};

    void record(FrameContext& frame) override;

    VkCommandBuffer getCommandBuffer(uint32_t frameIndex) const override;

    void createLocalFramebuffers() override;

    void resizeTargets(VkExtent2D newExtent) override;

    void updateSwapchain(Swapchain& swapchain_) override;

private:
    std::unique_ptr<PassCommandBuffers> commandBuffers;
	std::unique_ptr<PassTarget> passTarget;
  
	void createRenderPass() override;
	void createPassDescriptorSetLayout() override;

    void bindGlobalDescriptorSet(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const override;

    VkCommandBuffer beginCommandBuffer(uint32_t frameIndex);
	void beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex);
	void setupViewportAndScissor(VkCommandBuffer cmd);
	void renderScene(VkCommandBuffer cmd, FrameContext& frame);
	void endCommandBuffer(VkCommandBuffer cmd);


    Swapchain* swapchain;
    VkExtent2D extent;
};
