#pragma once

#include "RenderPassBase.h"

class ShadowMap;
class RenderSystem;

class ShadowPass final : public RenderPassBase {
public:
    ShadowPass(
        ShadowMap& shadowMap,
        RenderSystem& renderer
    );

    void execute(FrameContext& frame) override;

private:
    ShadowMap& shadowMap;
    RenderSystem& renderer;
};
