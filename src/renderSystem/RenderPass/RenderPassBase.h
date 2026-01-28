#pragma once

#include "../FrameContext.h"
#include "../../base/Device.h"

#include "../RenderSystems/BaseRenderSystem.h"

#include "../../assetManager/AssetManager.h"
#include "../RenderSystems/RenderSystemBuilder.h"

#include <vulkan/vulkan_core.h>
#include <vector>
#include <memory>

//class IRenderPass {
//public:
//    virtual ~IRenderPass() = default;
//
//    virtual void record(FrameContext& frame) = 0;
//
//    virtual VkCommandBuffer commandBuffer(uint32_t frameIndex) const = 0;
//
//protected:
//    IRenderPass() = default;
//
//    IRenderPass(const IRenderPass&) = delete;
//    IRenderPass& operator=(const IRenderPass&) = delete;
//};
//
//class RenderSystemGroup {
//public:
//    void add(std::unique_ptr<BaseRenderSystem> system) {
//        systems.emplace_back(std::move(system));
//    }
//
//    void record(
//        VkCommandBuffer cmd,
//        FrameContext& frame
//    ) {
//        for (auto& system : systems)
//            system->record(cmd, frame);
//    }
//
//private:
//    std::vector<std::unique_ptr<BaseRenderSystem>> systems;
//};
//
//struct PassTarget {
//    TextureManager::TextureID color;
//    TextureManager::TextureID depth;
//};
//
//class MainPassResources {
//public:
//    MainPassResources(
//        Device& device,
//        AssetManager& assets,
//        Swapchain& swapchain
//    );
//
//    ~MainPassResources();
//
//    void create();
//    void destroy();
//
//    void resize(VkExtent2D newExtent);
//
//    VkFramebuffer framebuffer(uint32_t imageIndex) const;
//    VkRenderPass renderPass() const;
//
//private:
//    void createRenderPass();
//    void createTargets();
//    void createFramebuffers();
//
//    void destroyFramebuffers();
//    void destroyTargets();
//
//private:
//    Device& device;
//    AssetManager& assets;
//    Swapchain* swapchain;
//
//    VkExtent2D extent;
//
//    VkRenderPass renderPass{ VK_NULL_HANDLE };
//    std::vector<PassTarget> targets;
//    std::vector<VkFramebuffer> framebuffers;
//};


class PassCommandBuffers {
public:
    PassCommandBuffers(Device& device, uint32_t nbCommandBuffer)
        : device(device)
    {
        buffers.resize(nbCommandBuffer);
        allocate();
    }

    VkCommandBuffer get(uint32_t frame) const {
        return buffers[frame];
    }

private:
    void allocate();
    Device& device;
    std::vector<VkCommandBuffer> buffers;
};

class PassTarget {

public:
    PassTarget(
        Device& device_,
        Swapchain& swapchain_,
		AssetManager& assets_,
        VkExtent2D extent_,
		bool isFinal = false
    )
        : device(device_), extent(extent_), assets(assets_) {

        createColorTargetTexture(swapchain_.imageCount(), swapchain_.format(), (isFinal) ? swapchain_.getImages() : std::vector<VkImage>{});
		createDepthTargetTexture(swapchain_.imageCount(), swapchain_.depthFormat());
    }

    std::vector<VkFramebuffer> framebuffers;

    void resizeTargets(VkExtent2D newExtent, uint32_t imageCount, VkFormat format, VkFormat depthFormat);

    void createLocalFramebuffers(VkRenderPass renderPass); 

private:
    void cleanupLocalFramebuffers();
    void cleanupTargetTextures();
   
	void createColorTargetTexture(uint32_t imageCount, VkFormat format, std::vector<VkImage> swapImage = {});
	void createDepthTargetTexture(uint32_t imageCount, VkFormat format);

    std::vector<TextureManager::TextureID> color;
    std::vector<TextureManager::TextureID> depth;

    VkExtent2D extent;

    AssetManager& assets;
	Device& device;
};



class RenderPassBase {
public:
    
    virtual ~RenderPassBase() = default; 

	RenderPassBase(Device& device_, AssetManager& assets_)
        : device(device_), assets(assets_) {
    }

    RenderPassBase(const RenderPassBase&) = delete;
    RenderPassBase& operator=(const RenderPassBase&) = delete;

    /**
     * record commands for this pass into command buffer
     */
    virtual void record(FrameContext& frame) = 0;

    /**
     * returns the command buffer for a given frame index
     */
    virtual VkCommandBuffer getCommandBuffer(uint32_t frameIndex) const = 0;

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
    void setAsFinal() { isFinalPass = true; }

    /**
	 * resize targets for the pass
     */
    virtual void resizeTargets(VkExtent2D newExtent) {};

	/**
     * update local reference to swapchain
	 */
    virtual void updateSwapchain(Swapchain& swapchain_) {};

protected:

    virtual void bindGlobalDescriptorSet(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const {}

    RenderPassBase() = default;

    bool isFinalPass = false;

    // Render systems used by this pass
    std::vector<std::unique_ptr<BaseRenderSystem>> renderSystems;

    // Timeline semaphore value signaled by this pass
    uint64_t signaledTimelineValue{ 0 };

    VkRenderPass renderPass{ VK_NULL_HANDLE };  

    std::unique_ptr<DescriptorSetLayout> setLayout;
    Device& device;
    AssetManager& assets;
};
