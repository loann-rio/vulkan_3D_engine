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

void FrameRenderer::recordPasses(FrameContext& frame) 
{
    for (auto& pass : passes) 
    {
        pass->record(frame);
    }

}

void FrameRenderer::submitPasses(FrameContext& frame) 
{
    for (auto& pass : passes) 
    {
        pass->submit(frame);
    }
}
