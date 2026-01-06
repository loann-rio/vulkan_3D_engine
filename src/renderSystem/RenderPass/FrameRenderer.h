#pragma once

#include <memory>
#include <vector>

#include "RenderPassBase.h"

class FrameRenderer {
public:
    explicit FrameRenderer(Device& device);

    void addPass(std::unique_ptr<RenderPassBase> pass);

    void render(FrameContext& frame);

private:
    void recordPasses(FrameContext& frame);
    void submitPasses(FrameContext& frame);

    Device& device;

    std::vector<std::unique_ptr<RenderPassBase>> passes;

};
