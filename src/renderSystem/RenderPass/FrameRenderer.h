#pragma once

#include <memory>
#include <vector>

#include "RenderPassBase.h"

struct SubmitSync {
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> signalSemaphores;
};

class FrameRenderer {
public:
    explicit FrameRenderer(Device& device);

    void addPass(std::unique_ptr<RenderPassBase> pass);
    
    void resizePasses(VkExtent2D newExtent);

	size_t getPassCount() const { return passes.size(); }
    
    RenderPassBase& getPass(size_t index) const { return *passes[index]; }
	RenderPassBase& getLastPass() const { return *passes.back(); }

    void recordPasses(FrameContext& frame);
    void submitPasses(FrameContext& frame);

private:

    SubmitSync buildSubmitSync(
        FrameContext& frame,
        bool isFirstPass,
        bool isLastPass
    );

    Device& device; 

    std::vector<std::unique_ptr<RenderPassBase>> passes;
};
