#pragma once

#include <memory>
#include <vector>

#include "RenderPassBase.h"

class FrameRenderer {
public:
    explicit FrameRenderer(Device& device);

    void addPass(std::unique_ptr<RenderPassBase> pass);

    void render(FrameContext& frame);

	size_t getPassCount() const { return passes.size(); }
    
    RenderPassBase& getPass(size_t index) const { return *passes[index]; }
	RenderPassBase& getLastPass() const { return *passes.back(); }

private:
    void recordPasses(FrameContext& frame);
    void submitPasses(FrameContext& frame);

    Device& device; 

    std::vector<std::unique_ptr<RenderPassBase>> passes;
};
