#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cstdint>

class Device;
class RenderPassBase;
class ModelAsset;
class IVertexLayout;
class Pipeline;

class BaseRenderSystem {
public:
    BaseRenderSystem(
        Device& device,
        RenderPassBase& renderPass
    );

    virtual ~BaseRenderSystem();

    BaseRenderSystem(const BaseRenderSystem&) = delete;
    BaseRenderSystem& operator=(const BaseRenderSystem&) = delete;

    /* ---- Lifecycle ---- */

    void create();
    void destroy();

    //// Recording ////

    void record(
        VkCommandBuffer cmd,
        const ModelAsset& model,
        uint32_t lodIndex
    ) const;

protected:

    /// Shader stages
    virtual VkShaderModule vertexShader() const = 0;
    virtual VkShaderModule fragmentShader() const = 0;

    /// Vertex layout compatibility
    virtual const IVertexLayout& vertexLayout() const = 0;

    /// Descriptor layouts used by this system
    virtual std::vector<VkDescriptorSetLayoutBinding>
        descriptorBindings() const = 0;

    /// Pipeline state specialization
    virtual void configurePipeline(Pipeline& pipeline) const = 0;

    /// bind model specific
    virtual void bindDescriptors(
        VkCommandBuffer cmd,
        const ModelAsset& model,
        uint32_t lodIndex
    ) const = 0;

    /// Issue draw calls
    virtual void drawModel(
        VkCommandBuffer cmd,
        const ModelAsset& model,
        uint32_t lodIndex
    ) const = 0;

protected:
    Device& device;
    RenderPassBase& renderPass;

    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};
