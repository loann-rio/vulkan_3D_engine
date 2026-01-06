#include "FrameRenderer.h"

#include <future>
#include <cassert>

FrameRenderer::FrameRenderer(Device& device) 
    : device{ device } { }

void FrameRenderer::addPass(std::unique_ptr<RenderPassBase> pass) 
{
    assert(pass && "Cannot add null render pass");
    passes.emplace_back(std::move(pass));
}

void FrameRenderer::render(FrameContext& frame)
{
    // record all passes
    recordPasses(frame);

    // submit passes in order
    submitPasses(frame);
}

void FrameRenderer::recordPasses(FrameContext& frame) {
    std::vector<std::future<void>> parallelJobs;

    for (auto& pass : passes) {
        if (pass->allowParallelRecording()) 
        {
            parallelJobs.emplace_back(
                std::async(std::launch::async, [&pass, &frame]() {
                    pass->record(frame);
                    })
            );
        }
        else {
            pass->record(frame);
        }
    }

    // Ensure all parallel recordings are complete
    for (auto& job : parallelJobs) {
        job.get();
    }
}

void FrameRenderer::submitPasses(FrameContext& frame) {
    for (auto& pass : passes) {
        pass->submit(frame);
    }
}
