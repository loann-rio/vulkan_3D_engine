//#include "ShadowPass.h"
//
//#include <execution>
//#include <algorithm>
//#include <numeric>
//
//#include "../SwapChain.h"
//
//ShadowPass::ShadowPass(Device& device, uint32_t frame_in_flight, VkRenderPass renderPass, VkExtent2D extent, uint32_t lightCount)
//	: RenderPassBase(device, frame_in_flight * lightCount), renderPass{ renderPass }, extent{ extent }, lightCount{ lightCount } {
//}
//
//void ShadowPass::addRenderSystem(std::unique_ptr<RenderSystem> system)
//{
//	renderSystems.push_back(system);
//}
//
//void ShadowPass::execute(FrameContext& frame)
//{
//    const uint32_t frameIndex = frame.frameIndex;
//
//    std::vector<VkSubmitInfo> submits;
//    submits.reserve(lightCount);
//
//    for (uint32_t light = 0; light < lightCount; ++light) {
//        VkCommandBuffer cmd = commandBuffers[frameIndex * Swapchain::MAX_FRAMES_IN_FLIGHT + light];
//
//        vkResetCommandBuffer(cmd, 0);
//        record(cmd, frame);
//
//        VkTimelineSemaphoreSubmitInfo timelineInfo{};
//        timelineInfo.sType =
//            VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
//        timelineInfo.signalSemaphoreValueCount = 1;
//        timelineInfo.pSignalSemaphoreValues = &++signalValue;
//
//        VkSubmitInfo submit{};
//        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        submit.pNext = &timelineInfo;
//        submit.commandBufferCount = 1;
//        submit.pCommandBuffers = &cmd;
//        submit.signalSemaphoreCount = 1;
//        submit.pSignalSemaphores = &frame.timeline;
//
//        submits.push_back(submit);
//    }
//
//    // Parallel GPU execution: multiple shadow maps
//    if (vkQueueSubmit(
//        device.graphicsQueue(),
//        static_cast<uint32_t>(submits.size()),
//        submits.data(),
//        VK_NULL_HANDLE
//    ) != VK_SUCCESS) {
//        throw std::runtime_error("ShadowPass: submit failed");
//    }
//}
//
//void ShadowPass::record(VkCommandBuffer cmd, FrameContext& frame)
//{
//    VkCommandBufferBeginInfo begin{};
//    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//
//    vkBeginCommandBuffer(cmd, &begin);
//
//    VkRenderPassBeginInfo rp{};
//    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//    rp.renderPass = renderPass;
//    rp.framebuffer = /* shadow framebuffer for this light */;
//    rp.renderArea.extent = extent;
//
//    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
//
//    for (auto& system : renderSystems) {
//        system->record(cmd, frame);
//    }
//
//    vkCmdEndRenderPass(cmd);
//    vkEndCommandBuffer(cmd);
//}
//
//VkPipelineStageFlags ShadowPass::waitStageMask() const
//{
//	return VkPipelineStageFlags();
//}
