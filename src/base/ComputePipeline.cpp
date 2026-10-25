#include "ComputePipeline.h"
#include <fstream>
#include <stdexcept>

ComputePipeline::ComputePipeline(
    Device& device, 
    const std::string& compFilepath, 
    VkPipelineLayout pipelineLayout)
    : device{ device } {

    createComputePipeline(compFilepath, pipelineLayout);
}

ComputePipeline::~ComputePipeline() 
{
    if (compShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device.device(), compShaderModule, nullptr);

    if (pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device.device(), pipeline, nullptr);
}

std::vector<char> ComputePipeline::readFile(const std::string& filepath) 
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
        throw std::runtime_error("Failed to open file: " + filepath);

    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);

    file.seekg(0);
    file.read(buffer.data(), size);
    
    file.close();
    return buffer;
}

void ComputePipeline::createComputePipeline(const std::string& compFilepath, VkPipelineLayout pipelineLayout) {
    assert(pipelineLayout != VK_NULL_HANDLE && "PipelineLayout required for compute pipeline");

    auto compCode = readFile(compFilepath);
    createShaderModule(compCode, &compShaderModule);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = compShaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(
        device.device(), 
        VK_NULL_HANDLE, 
        1, 
        &pipelineInfo, 
        nullptr, 
        &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline");
    }
}

void ComputePipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute shader module");
    }
}

void ComputePipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}