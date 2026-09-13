#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "device.h"

class ComputePipeline {
public:
    ComputePipeline(Device& device, const std::string& compFilepath, VkPipelineLayout pipelineLayout);
    ~ComputePipeline();

    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    void bind(VkCommandBuffer commandBuffer);

private:
    std::vector<char> readFile(const std::string& filepath);
    void createComputePipeline(const std::string& compFilepath, VkPipelineLayout pipelineLayout);
    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    Device& device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkShaderModule compShaderModule = VK_NULL_HANDLE;
};
