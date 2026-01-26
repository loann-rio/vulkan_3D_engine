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

void FrameRenderer::resizePasses(VkExtent2D newExtent)
{
    for (auto& pass : passes) {
        pass->resizeTargets(newExtent);
    }
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

        VkCommandBuffer cmd = pass->getCommandBuffer(frame.frameIndex);

        std::vector<VkSemaphore> waitSemaphores{};
        waitSemaphores.push_back(frame.swapchain->getImageAvailableSemaphore(frame.frameIndex));

        std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submit.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
        submit.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

		std::vector<VkSemaphore> signalSemaphores{};
        if (i == passCount - 1) {
            // last pass signals render finished semaphore
            signalSemaphores.push_back(frame.swapchain->getRenderFinishedSemaphore(frame.frameIndex));
        }
        else {
			throw std::runtime_error("FrameRenderer::submitPasses: Intermediate passes not supported yet");
            // intermediate pass signals timeline semaphore
            signalSemaphores.push_back(frame.timeline);
		}

        submit.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submit.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

        device.submitToGraphicQueue(submit, frame.swapchain->getInFlightFence(frame.frameIndex));
    }

}
