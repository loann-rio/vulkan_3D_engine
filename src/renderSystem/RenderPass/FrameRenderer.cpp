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

        //////// timeline semaphore info ////////

        uint64_t signalValue = ++frame.timelineValue;

        VkTimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &signalValue;

        uint64_t waitValue = lastTimelineValue;

        if (!isFirstPass)
        {
            timelineInfo.waitSemaphoreValueCount = 1;
            timelineInfo.pWaitSemaphoreValues = &waitValue;
        }

        //////// wait semaphores ////////
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
            waitStages.push_back(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        ////////// signal semaphores ////////
        //std::vector<VkSemaphore> signalSemaphores;
        //signalSemaphores.push_back(frame.timelineSemaphore);

        //if (isLastPass)
        //{
        //    signalSemaphores.push_back(
        //        frame.swapchain->getRenderFinishedSemaphore(frame.frameIndex));
        //}


		////// submit info ////////

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = &timelineInfo;
            
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &frame.timelineSemaphore;

        submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submit.pWaitSemaphores = waitSemaphores.data();
        submit.pWaitDstStageMask = waitStages.data();
            
        submit.commandBufferCount = static_cast<uint32_t>(cmds.size());
        submit.pCommandBuffers = cmds.data();

        device.submitToGraphicQueue(submit, VK_NULL_HANDLE);
        
        lastTimelineValue = signalValue;

        if (isLastPass)
        {
            VkSemaphore waitSemaphore = frame.timelineSemaphore;
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

            VkSubmitInfo presentSubmit{};
            presentSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            presentSubmit.waitSemaphoreCount = 1;
            presentSubmit.pWaitSemaphores = &waitSemaphore;
            presentSubmit.pWaitDstStageMask = &waitStage;

            VkSemaphore signal = frame.swapchain
                ->getRenderFinishedSemaphore(frame.frameIndex);

            presentSubmit.signalSemaphoreCount = 1;
            presentSubmit.pSignalSemaphores = &signal;

            presentSubmit.commandBufferCount = 0;

            device.submitToGraphicQueue(
                presentSubmit,
                frame.swapchain->getInFlightFence(frame.frameIndex));
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
