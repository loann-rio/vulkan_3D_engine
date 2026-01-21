#include "FrameRenderer.h"

#include <future>
#include <cassert>

FrameRenderer::FrameRenderer(Device& device) 
    : device{ device } { }

void FrameRenderer::addPass(std::unique_ptr<RenderPassBase> pass) 
{
    assert(pass && "cannot add null render pass");
    passes.emplace_back(std::move(pass));
}

void FrameRenderer::render(FrameContext& frame)
{
    // record all passes
    recordPasses(frame);

    // submit passes in order
    submitPasses(frame);
}

void FrameRenderer::recordPasses(FrameContext& frame) 
{
    for (auto& pass : passes) 
    {
        pass->record(frame);
    }

}

void FrameRenderer::submitPasses(FrameContext& frame) 
{
    const uint64_t frameSignalValue = frame.timelineValue;
    const size_t passCount = passes.size();

    if (frame.timeline == VK_NULL_HANDLE) {
        throw std::runtime_error("FrameContext.timeline is VK_NULL_HANDLE");
    }

    if (frameSignalValue == 0) {
        throw std::runtime_error("invalid timeline signal value: 0");
    }

    for (size_t i = 0; i < passCount; ++i)
    {
        auto& pass = passes[i];

        VkCommandBuffer cmd = pass->commandBuffer(frame.frameIndex);

        // Build wait semaphores
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<uint64_t> waitValues; // timeline values

        // swapchain image availability
        if (i == 0 && frame.imageAvailable != VK_NULL_HANDLE) {
            waitSemaphores.push_back(frame.imageAvailable);
            waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            waitValues.push_back(0);
        }


        // Timeline dependency
        if (i == 0)
        {
            waitSemaphores.push_back(frame.timeline);
            waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            waitValues.push_back(frameSignalValue - 1); // previous frame
        }

        std::vector<VkSemaphore> signalSemaphores;
        std::vector<uint64_t> signalValues;


        // signal timeline only on last pass
        if (i + 1 == passCount)
        {
            signalSemaphores.push_back(frame.timeline);
            signalValues.push_back(frameSignalValue);

            if (frame.swapchain)
            {
                signalSemaphores.push_back(
                    frame.swapchain->getRenderFinishedSemaphore(frame.frameIndex)
                );
                signalValues.push_back(0); // placeholder for binary semaphore
            }
        }


        VkTimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        timelineInfo.pWaitSemaphoreValues = waitValues.data();
        timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        timelineInfo.pSignalSemaphoreValues = signalValues.data();

        assert(timelineInfo.waitSemaphoreValueCount == waitValues.size());
        assert(timelineInfo.signalSemaphoreValueCount == signalValues.size());

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = &timelineInfo;
        submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submit.pWaitSemaphores = waitSemaphores.data();
        submit.pWaitDstStageMask = waitStages.data();
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submit.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

        device.submitToGraphicQueue(submit, VK_NULL_HANDLE);

    }

}
