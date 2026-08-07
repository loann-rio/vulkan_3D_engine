#include "GlobalRenderSystem.h"
#include "../base/Device.h"
#include "../assetManager/AssetManager.h"
#include <vulkan/vulkan_core.h>

class GlobalRenderSystemBuilder
{
public:

    GlobalRenderSystemBuilder(Device& device, AssetManager& assets)
        : device(device), assets(assets)
    {}

    GlobalRenderSystemBuilder& renderPass(VkRenderPass pass) { config.renderPass = pass; return *this; }
    GlobalRenderSystemBuilder& vertexShader(std::string shader) { config.vertexShader = std::move(shader); return *this; }
    GlobalRenderSystemBuilder& fragmentShader(std::string shader) { config.fragmentShader = std::move(shader); return *this; }
    GlobalRenderSystemBuilder& modelFilterType(ModelType type) { config.modelType = type; return *this; }
    GlobalRenderSystemBuilder& modelSubType(ModelSubType type) { config.modelSubType = type; return *this; }
    GlobalRenderSystemBuilder& fullscreen() { config.fullscreen = true; return *this; }
    GlobalRenderSystemBuilder& shadow() { config.shadow = true; return *this; }
    GlobalRenderSystemBuilder& skybox() { config.skybox = true; return *this; }
    GlobalRenderSystemBuilder& addSetLayout(VkDescriptorSetLayout set) { config.globalLayouts.push_back(set); return *this; }
    GlobalRenderSystemBuilder& bindingDescriptions(std::vector<VkVertexInputBindingDescription> bindings) { config.bindingDescriptions = bindings; return *this; }
    GlobalRenderSystemBuilder& attributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributeDescriptions) { config.attributeDescriptions = attributeDescriptions; return *this; }
    GlobalRenderSystemBuilder& descriptorBindings(std::vector<DescriptorSetObject> descriptorBindings) { config.descriptorBindings = descriptorBindings; return *this; }
    GlobalRenderSystemBuilder& pushStage(VkShaderStageFlags pushStage) { config.pushStage = pushStage; return *this; }

    std::unique_ptr<GlobalRenderSystem> build()
    {
        // assert:
        // render Pass
        // set layout (or enable empty)
        // shader
        // model type
        // sub type
        // binding
        // attribute
        // descriptor
        // push stage
        // is compatible with type of render

        config.modelDescriptorSetIndex = static_cast<uint16_t>(config.globalLayouts.size());


        return std::make_unique<GlobalRenderSystem>(
            device,
            assets,
            config
        );
    }

private:

    Device& device;
    AssetManager& assets;

    RenderSystemConfig config;
};