#pragma once

#include "RenderPassBase.h"
#include "../../base/Device.h"
#include "../FrameContext.h"
#include "../SwapChain.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>


class RenderSystem;

/*

Main color rendering pass

record command buffer
bind framebuffer
execute all renderSystems

*/
class MainPass final : public RenderPassBase {
public:
    MainPass(
        Device& device,
        AssetManager& assets,
        Swapchain& swapchain,
        uint32_t frame_in_flight, 
        VkExtent2D extent
    );

    ~MainPass() override;

    void record(FrameContext& frame) override;

    VkCommandBuffer commandBuffer(uint32_t frameIndex) const override;



private:

    void allocateCommandBuffers(uint32_t framesInFlight);
    
    void createTargetTexture();
    void createFramebuffers();

	void createRenderPass() override;

    VkCommandBuffer beginPass(uint32_t frameIndex);

    Device& device;
    Swapchain& swapchain;
    AssetManager& assets;
    VkExtent2D extent;

    

    std::vector<VkFramebuffer> framebuffers;
    std::vector<TextureManager::TextureID> textureTarget;
};
