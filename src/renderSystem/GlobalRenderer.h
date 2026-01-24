#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "RenderPass/FrameRenderer.h"


class Device;
class Swapchain;

struct GlobalUbo_ {
    glm::mat4 projection{ 1.0f };
    glm::mat4 view{ 1.0f };
    glm::mat4 inverseView{ 1.f };

    // cam position
    // global light dir

    glm::vec4 ambientLightColor{ 1.f, 1.f,  1.f, .1f };
    glm::vec4 globalLightDir{ 1.f, -3.f, 0.5f, 0.f };
};

class GlobalRenderer {
public:
    GlobalRenderer(Device& device, Window& window, AssetManager& assetManager, ObjectManager& objectManager);
    ~GlobalRenderer();

    GlobalRenderer(const GlobalRenderer&) = delete;
    GlobalRenderer& operator=(const GlobalRenderer&) = delete;

    void renderFrame(std::vector<GameObjectModel*> listGameObjects);

    GlobalUbo_ ubo{};

private:

    void createUboDescriptorPool();
    
    void createFrameRenderer();
	void createFrameBuffers();

    void createGlobalUniformBuffer();

    void recreateSwapchain();

    bool aquireNextImage();
    void presentFrame();

	void updateGlobalUniformBuffer(uint32_t frameIndex);



private:
    Device& device;
    Window& window;
    AssetManager& assetManager;
    ObjectManager& objectManager;

    std::unique_ptr<DescriptorPool> globalPool; 

    FrameContext frameContext{};

    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<FrameRenderer> frameRenderer;

    uint32_t currentImageIndex;
    int currentFrameIndex = 0; 

    /// Global UBO
    std::vector<std::unique_ptr<Buffer>> uboBuffers; 
    std::vector<VkDescriptorSet> globalDescriptorSet;

};



