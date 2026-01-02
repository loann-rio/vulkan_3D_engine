#pragma once

#include <memory>
#include <vector>

#include "RenderPassBase.h"

class FrameRenderer {
public:
    void addPass(std::unique_ptr<RenderPassBase> pass);

    void render(FrameContext& frame);

private:
    std::vector<std::unique_ptr<RenderPassBase>> passes;
};
