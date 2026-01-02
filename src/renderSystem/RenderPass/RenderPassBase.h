#pragma once

#include "FrameContext.h"

class RenderPassBase {
public:
    virtual ~RenderPassBase() = default;

    virtual void execute(FrameContext& frame) = 0;
};
