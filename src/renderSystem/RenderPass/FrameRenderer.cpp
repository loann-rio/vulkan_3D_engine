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
    const size_t passCount = passes.size();
    for (size_t i = 0; i < passCount; ++i)
    {
        auto& pass = passes[i];
        pass->resizeTargets(newExtent);
        pass->createLocalFramebuffers();
    }

    auto& lastPass = passes.back();
    lastPass->resizeTargets(newExtent);
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

    uint64_t lastTimelineValue = frame.timelineValue;

    const size_t passCount = passes.size();
    for (size_t i = 0; i < passCount; ++i)
    {
        auto& pass = passes[i];

		std::vector<VkCommandBuffer> cmds = pass->getFrameCommandBuffers(frame.frameIndex);

        if (cmds.empty())
            continue;

        const bool isFirstPass = (i == 0);
        const bool isLastPass = (i == passCount - 1);

        for (VkCommandBuffer cmd : cmds)
        {

            uint64_t signalValue = ++frame.timelineValue;
            lastTimelineValue = signalValue;


            VkTimelineSemaphoreSubmitInfo timelineInfo{};
            timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            timelineInfo.signalSemaphoreValueCount = 1;
            timelineInfo.pSignalSemaphoreValues = &signalValue;

            uint64_t waitValue = lastTimelineValue - 1;
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            if (!isFirstPass)
            {
                timelineInfo.waitSemaphoreValueCount = 1;
                timelineInfo.pWaitSemaphoreValues = &waitValue;
            }

            std::vector<VkSemaphore> waitSemaphores{};
            std::vector<VkPipelineStageFlags> waitStages{};

            if (isFirstPass)
            {
                waitSemaphores.push_back(
                    frame.swapchain->getImageAvailableSemaphore(frame.frameIndex));
                waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            }
            else
            {
                waitSemaphores.push_back(frame.timelineSemaphore);
                waitStages.push_back(waitStage);
            }

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
                signalSemaphores.push_back(frame.timelineSemaphore);
            }

            submit.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
            submit.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

            device.submitToGraphicQueue(submit, frame.swapchain->getInFlightFence(frame.frameIndex));
        }
    }

}

SubmitSync FrameRenderer::buildSubmitSync(FrameContext& frame, bool isFirstPass, bool isLastPass)
{
    SubmitSync sync{};

    if (isFirstPass) {
        sync.waitSemaphores.push_back(
            frame.swapchain->getImageAvailableSemaphore(frame.frameIndex));
        sync.waitStages.push_back(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    }
    else {
        sync.waitSemaphores.push_back(frame.timelineSemaphore);
        sync.waitStages.push_back(
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    sync.signalSemaphores.push_back(frame.timelineSemaphore);

    if (isLastPass) {
        sync.signalSemaphores.push_back(
            frame.swapchain->getRenderFinishedSemaphore(frame.frameIndex));
    }

    return sync;
}
