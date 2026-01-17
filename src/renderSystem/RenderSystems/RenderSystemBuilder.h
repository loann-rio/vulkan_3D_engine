#pragma once

#include <vulkan/vulkan.h>
#include <string>

#include "ShadowMeshRenderSystem.h"
#include "ShadowSkinnedRenderSystem.h"
#include "MainMeshRenderSystem.h"
#include "MainSkinnedRenderSystem.h"

#include "BaseRenderSystem.h"
#include "../../base/Device.h"


class RenderSystemBuilder {
public:
    RenderSystemBuilder& vertexShader(const std::string& path) {
        config.vertexShaderPath = path;
        return *this;
    }

    RenderSystemBuilder& fragmentShader(const std::string& path) {
        config.fragmentShaderPath = path;
        return *this;
    }

    RenderSystemBuilder& alphaBlend(bool enable) {
        config.alphaBlend = enable;
        return *this;
    }

    RenderSystemBuilder& cullMode(VkCullModeFlags mode) {
        config.cullMode = mode;
        return *this;
    }

    RenderSystemBuilder& renderPass(VkRenderPass pass) {
        config.renderPass = pass;
        return *this;
	}

    RenderSystemBuilder& enableSkinning(bool enable) {
        skinningEnable = enable;
        return *this;
    }

    RenderSystemBuilder& vertexLayout(IVertexLayout* layout_) {
        layout = layout_;
        return *this;
    }

    RenderSystemBuilder& asMainRenderSystem() {
		asMain = true;
        asShadow = false;
        return *this;
	}

	RenderSystemBuilder& asShadowRenderSystem() {
        asMain = false;
        asShadow = true;
		return *this;
	}

    RenderSystemBuilder& setGlobalSetLayout(DescriptorSetLayout* layout_) {
        config.globalSetLayout = layout_;
        return *this;
	}

    std::unique_ptr<BaseRenderSystem> build(Device& device, AssetManager& assets) {
        if (asShadow) {
            //return buildShadow(device);
			throw std::runtime_error("RenderSystemBuilder: shadow render system not implemented yet");
        }
		return buildMain(device, assets);
    }

    /*std::unique_ptr<BaseRenderSystem> buildShadow(Device& device) {
        if (skinningEnable) { 
            return std::make_unique<ShadowSkinnedRenderSystem>(device, layout);
        }
        return std::make_unique<ShadowMeshRenderSystem>(device, layout);
    }*/

    std::unique_ptr<BaseRenderSystem> buildMain(Device& device, AssetManager& assets) {
        if (!layout) throw std::runtime_error("RenderSystemBuilder: vertexLayout not set");
        /*if (skinningEnable) {
            return std::make_unique<MainSkinnedRenderSystem>(device, layout);
        }*/
        return std::make_unique<MainMeshRenderSystem>(device, assets, *layout, config);
    }

private:

    bool asMain = true;
	bool asShadow = false;

    RenderSystemCreateInfo config;

    IVertexLayout* layout = nullptr;
    bool skinningEnable = false;
};