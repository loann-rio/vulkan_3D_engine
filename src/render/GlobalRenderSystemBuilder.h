#include "RenderSystem.h"
#include "../base/Device.h"
#include "../assetManager/AssetManager.h"
#include <vulkan/vulkan_core.h>

namespace {
    std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescription(std::vector<IVertexLayout::Attribute> attributes) {

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        uint16_t i = 0;
        for (auto element : attributes) {
            VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
            if (element.size == 12Ui64) format = VK_FORMAT_R32G32B32_SFLOAT;
            if (element.size == 8Ui64) format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions.push_back({ i++, 0, format, element.offset });
        }

        return attributeDescriptions;

    }

}


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
    GlobalRenderSystemBuilder& skybox() { config.skybox = true; config.modelSubType = ModelSubType::SKYBOX; return *this; }
    GlobalRenderSystemBuilder& addSetLayout(VkDescriptorSetLayout set) { config.globalLayouts.push_back(set); return *this; }
    GlobalRenderSystemBuilder& bindingDescriptions(std::vector<VkVertexInputBindingDescription> bindings) { config.bindingDescriptions = bindings; return *this; }
    GlobalRenderSystemBuilder& attributeDescriptions(std::vector<VkVertexInputAttributeDescription> attributeDescriptions) { config.attributeDescriptions = attributeDescriptions; return *this; }
    GlobalRenderSystemBuilder& descriptorBindings(std::vector<DescriptorSetObject> descriptorBindings) { config.descriptorBindings = descriptorBindings; return *this; }
    GlobalRenderSystemBuilder& pushStage(VkShaderStageFlags pushStage) { config.pushStage = pushStage; return *this; }

    template<class Vertex>
    std::unique_ptr<RenderSystem> build()
    {
        std::vector<DescriptorSetObject> descriptorBindings;
        std::vector<VkVertexInputAttributeDescription> attributeDescription;
        std::vector<VkVertexInputBindingDescription> bindingDescription; 

        ModelType modelType = static_cast<ModelType>(config.modelType);

        Vertex vertex{};

        // Only populate vertex binding/attribute descriptions if the pipeline needs vertex input
        if (!config.fullscreen) {
            bindingDescription = getBindingDescriptions<Vertex>();

            if (config.shadow) {
                descriptorBindings = getDescriptorType();
                attributeDescription = getVertexInputAttributeDescription(std::vector<IVertexLayout::Attribute>{ vertex.attributes()[0] });
            }
            else {
                descriptorBindings = getDescriptorType();
                attributeDescription = getVertexInputAttributeDescription(vertex.attributes());
            }
        }
        else {
            // fullscreen: still may need descriptor bindings
            descriptorBindings = getDescriptorType();
            // leave bindingDescription and attributeDescription empty
        }

        if (config.attributeDescriptions.empty()) config.attributeDescriptions = attributeDescription;
        if (config.bindingDescriptions.empty())   config.bindingDescriptions = bindingDescription;
        if (config.descriptorBindings.empty())   config.descriptorBindings = descriptorBindings;
        if (config.modelType == ModelType::UNDEFINED_MODEL) config.modelType = modelType;

        config.modelDescriptorSetIndex = static_cast<uint16_t>(config.globalLayouts.size());

        assert(testRendererValidity() && "unknow error durring render system build");
        
        return std::make_unique<RenderSystem>(
            device,
            assets,
            config
        );
    }

private:

    bool testRendererValidity() {
        assert(config.renderPass != VK_NULL_HANDLE && "render pass should always be defined to create a render system");
        assert(!config.vertexShader.empty() && "vertex shader should not be empty");

        if (config.shadow)
            assert(config.fragmentShader.empty() && "fragment shader has be empty for shadow rendering");
        else
            assert(!config.fragmentShader.empty() && "fragment shader should not be empty");

        assert(config.modelType != ModelType::UNDEFINED_MODEL);

        assert(!(config.shadow && config.fullscreen) && "render system cannot be shadow and fullscreen at the same time");
        assert(!(config.skybox && config.fullscreen) && "render system cannot be skybox and fullscreen at the same time");

        return true;
    }

    template <class Vertex>
    std::vector<VkVertexInputBindingDescription> getBindingDescriptions()
    {
        Vertex vertex{};
        std::vector<VkVertexInputBindingDescription> bindingDescription(1);
        bindingDescription[0].binding = 0;
        bindingDescription[0].stride = vertex.stride();
        bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        //if (hasMutipleInstances)
        //bindingDescription.push_back({ 1, sizeof(ModelInstance), VK_VERTEX_INPUT_RATE_INSTANCE });

        return bindingDescription;
    }

    std::vector<DescriptorSetObject> getDescriptorType()
    {
        std::vector<DescriptorObject> set1 = {
             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
        };

        return std::vector<DescriptorSetObject>{{set1, 2}};
    }


    Device& device;
    AssetManager& assets;

    RenderSystemConfig config;
};