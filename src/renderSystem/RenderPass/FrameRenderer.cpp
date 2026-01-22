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
    const size_t passCount = passes.size();
    for (size_t i = 0; i < passCount; ++i)
    {
        auto& pass = passes[i];

        VkCommandBuffer cmd = pass->commandBuffer(frame.frameIndex);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = &timelineInfo;
        submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submit.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
        submit.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submit.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();


        device.submitToGraphicQueue(submit, VK_NULL_HANDLE);

    }

}
