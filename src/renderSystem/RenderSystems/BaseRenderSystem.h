#pragma once

#include "../FrameContext.h"
#include "../../base/Pipeline.h"
#include "../../assetManager/AssetManager.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cstdint>
#include <span>


class Device;
class ModelAsset;
class IVertexLayout;

struct RenderSystemCreateInfo {
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    bool depthTest = true;
    bool depthWrite = true;
    VkCompareOp depthCompare = VK_COMPARE_OP_LESS;

    bool alphaBlend = false;

    DescriptorSetLayout* globalSetLayout; 
};



class BaseRenderSystem {
public:
   BaseRenderSystem(
        Device& device_, 
        AssetManager& assets_, 
        const IVertexLayout& vertexLayout_, 
        const RenderSystemCreateInfo& createInfo_);

    ~BaseRenderSystem();

    BaseRenderSystem(const BaseRenderSystem&) = delete;
    BaseRenderSystem& operator=(const BaseRenderSystem&) = delete;

	/// determine if this render system can render the given object
    virtual bool accepts(
        const GameObjectModel& object
    ) const = 0;

    //// recording ////

    void record(
        VkCommandBuffer cmd,
        FrameContext& frameContext,
        const std::vector<RenderItem>& items
    ) const;

    void renderFullScreen(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const;

    VkPipelineLayout getPipelineLayout() const { 
        return pipelineLayout; 
    }

protected:
    struct PushConstantInfo {
        VkShaderStageFlags stages;
        uint32_t size;
    };

    virtual PushConstantInfo pushConstants() const = 0;
   
    /// descriptor layouts used by this system
    virtual void createDescriptorSetLayouts(
        std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
    ) {};
    
    /// renderSystem-specific pipeline config
    virtual void configurePipeline(
        PipelineConfigInfo& pipelineConfig
    ) const {}


    /// render model: bind + draw
    virtual void renderModel(
        VkCommandBuffer cmd,
        FrameContext& frameContext,
        const RenderItem& item
    ) const = 0;

protected:
    Device& device;
    AssetManager& assets;
    const IVertexLayout& vertexLayout;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> pipeline;

    std::vector<std::unique_ptr<DescriptorSetLayout>> descriptorLayouts;

    void createPipelineLayout(DescriptorSetLayout* globalSetLayout);
    void createPipeline(const RenderSystemCreateInfo& createInfo);

    void configureVertexInput(PipelineConfigInfo& pipelineConfig) const;
    void bindPipeline(VkCommandBuffer cmd) const;

    RenderSystemCreateInfo createInfo;


};
