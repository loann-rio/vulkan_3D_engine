//#pragma once
//
//#include "RenderPassBase.h"
//
//#include <vulkan/vulkan.h>
//#include <vector>
//#include <memory>
//
//class RenderSystem;
//
//class ShadowPass final : public RenderPassBase {
//public:
//    ShadowPass(
//        Device& device,
//        uint32_t frame_in_flight, 
//        VkRenderPass renderPass,
//        VkExtent2D extent,
//        uint32_t lightCount
//    );
//
//    void addRenderSystem(std::unique_ptr<RenderSystem> system);
//
//    void execute(FrameContext& frame);
//
//
//private:
//    void record(VkCommandBuffer cmd, FrameContext& frame);
//
//    VkPipelineStageFlags waitStageMask() const override;
//
//    VkRenderPass renderPass{};
//    VkExtent2D extent{};
//
//    uint32_t lightCount{};
//
//    std::vector<std::unique_ptr<RenderSystem>> renderSystems;
//};
