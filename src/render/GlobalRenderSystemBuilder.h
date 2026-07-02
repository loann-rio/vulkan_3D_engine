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

    GlobalRenderSystemBuilder& renderPass(VkRenderPass pass)
    {
        config.renderPass = pass;
        return *this;
    }

    GlobalRenderSystemBuilder& vertexShader(std::string shader)
    {
        config.vertexShader = std::move(shader);
        return *this;
    }

    GlobalRenderSystemBuilder& fragmentShader(std::string shader)
    {
        config.fragmentShader = std::move(shader);
        return *this;
    }

    GlobalRenderSystemBuilder& modelType(ModelType type)
    {
        config.modelType = type;
        return *this;
    }

    GlobalRenderSystemBuilder& modelSubType(ModelSubType type)
    {
        config.modelSubType = type;
        return *this;
    }

    GlobalRenderSystemBuilder& fullscreen()
    {
        config.fullscreen = true;
        return *this;
    }

    GlobalRenderSystemBuilder& shadow()
    {
        config.shadow = true;
        return *this;
    }

    GlobalRenderSystemBuilder& skybox()
    {
        config.skybox = true;
        return *this;
    }

    std::unique_ptr<GlobalRenderSystem> build()
    {
        return std::make_unique<GlobalRenderSystem>(
            device,
            assets,
            config);
    }

private:

    Device& device;
    AssetManager& assets;

    RenderSystemConfig config;
};