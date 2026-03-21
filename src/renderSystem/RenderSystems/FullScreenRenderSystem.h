#pragma once

#include "BaseRenderSystem.h"


class FullScreenRenderSystem : public BaseRenderSystem {
    struct alignas(16) PushConstantData {};

public:
    FullScreenRenderSystem(
        Device& device,
        AssetManager& assets,
        const IVertexLayout* vertexLayout,
        const RenderSystemCreateInfo& createInfo
    );

    ~FullScreenRenderSystem() = default;

    bool accepts(
        const GameObjectModel& object
    ) const override;

protected:
    PushConstantInfo pushConstants() const override;

    void createDescriptorSetLayouts(
        std::vector<std::unique_ptr<DescriptorSetLayout>>& outLayouts
    ) override;

    void renderModel(
        VkCommandBuffer cmd,
        FrameContext& frameContext,
        const RenderItem& item
    ) const override;

    void renderFullScreen(
        VkCommandBuffer cmd, 
        FrameContext& frameContext
    ) const override;

    void configurePipeline(
        PipelineConfigInfo& pipelineConfig
    ) const override;
};