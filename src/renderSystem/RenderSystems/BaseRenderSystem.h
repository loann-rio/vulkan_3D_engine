#pragma once

#include "../FrameContext.h"
#include "../../base/Pipeline.h"
#include "../../assetManager/AssetManager.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cstdint>

class Device;
class RenderPassBase;
class ModelAsset;
class IVertexLayout;

struct RenderSystemConfig {
    // Shaders
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // Raster state
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Depth
    bool depthTest = true;
    bool depthWrite = true;
    VkCompareOp depthCompare = VK_COMPARE_OP_LESS;

    // Blending
    bool alphaBlend = false;

    // Pass type
    bool depthOnly = false;

    // Features
    bool hasMultipleInstance = false;

    // render pass
    VkRenderPass renderPass;

};


class BaseRenderSystem {
public:
    BaseRenderSystem(
        Device& device,
        AssetManager& assets,
        IVertexLayout* layout, 
        RenderSystemConfig& config
    );

    ~BaseRenderSystem();

    BaseRenderSystem(const BaseRenderSystem&) = delete;
    BaseRenderSystem& operator=(const BaseRenderSystem&) = delete;

    //// Recording ////

    void record(
        VkCommandBuffer cmd,
        FrameContext& frameContext
    ) const;

protected:
    /// Vertex layout compatibility
    const IVertexLayout& vertexLayout() const;

    // push constant
    virtual VkShaderStageFlagBits pushStage() const = 0;
    virtual uint32_t pushSize() const = 0;

   
    /// Descriptor layouts used by this system
    virtual std::vector<VkDescriptorSetLayoutBinding>
        descriptorBindings() const = 0;
    
    /// renderSystem-specific pipeline config
    virtual void configurePipeline(
        PipelineConfigInfo& pipelineConfig
    ) const = 0;

    /// add model descriptor set to global ones
    virtual std::vector<VkDescriptorSetLayout>
        createSetLayout(
            RenderSystemConfig& config
        ) = 0;

    /// render model: bind + draw
    virtual void renderModel(
        VkCommandBuffer cmd,
        FrameContext& frameContext,
        const ModelAsset& model,
        uint32_t lodIndex, 
        glm::mat4 modelMat, glm::mat4 normalM
    ) const = 0;

    /// bind model specific
    /*virtual void bindModelDescriptors(
        VkCommandBuffer cmd,
        const ModelAsset& model,
        uint32_t lodIndex
    ) const = 0;*/

    /// Issue draw calls
    virtual void drawModel(
        VkCommandBuffer cmd,
        const ModelAsset& model,
        FrameContext& frameContext,
        uint32_t lodIndex, uint32_t frameIndex,
        glm::mat4 modelMat, glm::mat4 normalM
    ) const = 0;

    

protected:
    Device& device;
    AssetManager& assets;

    IVertexLayout* vertexLayout_;
      
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;


private:
    void bindPipeline(
        VkCommandBuffer cmd
    ) const;

    /// Pipeline state specialization
    void createPipelineLayout(
        RenderSystemConfig& config,
        std::vector<VkDescriptorSetLayout> descriptorSetLayout
    );

    /// create full pipeline
    void createPipeline(
        RenderSystemConfig& config
    );

    /// add vertex binding to pipeline
    void configVertexBindingDescription(
        PipelineConfigInfo& pipelineConfig
    );

    /// add vertex attribute to pipeline
    void configVertexAttributeDescription(
        PipelineConfigInfo& pipelineConfig
    );


};
