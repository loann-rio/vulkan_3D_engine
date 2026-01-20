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
        DescriptorPool& renderPool,
        uint32_t frame_in_flight, 
        VkExtent2D extent
    );

    ~MainPass() override;

    void record(FrameContext& frame) override;

    VkCommandBuffer commandBuffer(uint32_t frameIndex) const override;

    void createLocalFramebuffers() override;

private:

    void allocateCommandBuffers(uint32_t framesInFlight);
    
    void createTargetTexture();

	void createRenderPass() override;
	void createPassDescriptorSetLayout() override;

    void bindGlobalDescriptorSet(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const override;

	void createGlobalUniformBuffer(DescriptorPool& renderPool);

    VkCommandBuffer beginPass(uint32_t frameIndex);

    Swapchain& swapchain;
    
    VkExtent2D extent;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<TextureManager::TextureID> textureTarget;
    
	/// Global UBO for the pass
    std::vector<std::unique_ptr<Buffer>> uboBuffers;
    std::vector<VkDescriptorSet> globalDescriptorSet;
};


struct GlobalUbo { 
    glm::mat4 projection{ 1.0f };
    glm::mat4 view{ 1.0f };
    glm::mat4 inverseView{ 1.f };
    glm::vec4 ambientLightColor{ 1.f, 1.f,  1.f, .1f };
    glm::vec4 globalLightDir{ 1.f, -3.f, 0.5f, 0.f };
};