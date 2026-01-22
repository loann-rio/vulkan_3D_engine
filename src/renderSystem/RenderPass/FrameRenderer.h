#pragma once

#include <memory>
#include <vector>

#include "RenderPassBase.h"

class FrameRenderer {
public:
    explicit FrameRenderer(Device& device);

    void addPass(std::unique_ptr<RenderPassBase> pass);

	size_t getPassCount() const { return passes.size(); }
    
    RenderPassBase& getPass(size_t index) const { return *passes[index]; }
	RenderPassBase& getLastPass() const { return *passes.back(); }


    void recordPasses(FrameContext& frame);
    void submitPasses(FrameContext& frame);

private:

    Device& device; 

    std::vector<std::unique_ptr<RenderPassBase>> passes;
};
