#pragma once

#include <vulkan/vulkan.h>
#include <memory>

class Device;
class Pipeline;
struct FrameContext;

struct PipelineConfig {
    VkRenderPass renderPass{};
    bool enableAlphaBlend{ false };
    bool depthOnly{ false };
};

class RenderSystem {
public:
    RenderSystem(
        Device& device,
        const PipelineConfig& config
    );

    ~RenderSystem();

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void draw(FrameContext& frame);

private:
    void createPipelineLayout();
    void createPipeline(const PipelineConfig& config);

private:
    Device& device;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout{};
};
