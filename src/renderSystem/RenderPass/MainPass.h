#pragma once

#include "RenderPassBase.h"

class Swapchain;
class RenderSystem;

class MainPass final : public RenderPassBase {
public:
    MainPass(
        Swapchain& swapchain,
        RenderSystem& renderer
    );

    void execute(FrameContext& frame) override;

private:
    Swapchain& swapchain;
    RenderSystem& renderer;
};
