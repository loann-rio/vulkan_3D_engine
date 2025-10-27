#include "GameObject.h"

 
GameObject::id_t GameObjectFactory::nextId = 0;

glm::mat4 TransformComponent::mat4() 
{
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    return glm::mat4{
        {
            scale.x * (c1 * c3 + s1 * s2 * s3),
            scale.x * (c2 * s3),
            scale.x * (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
            scale.y * (c3 * s1 * s2 - c1 * s3),
            scale.y * (c2 * c3),
            scale.y * (c1 * c3 * s2 + s1 * s3),
            0.0f,
        },
        {
            scale.z * (c2 * s1),
            scale.z * (-s2),
            scale.z * (c1 * c2),
            0.0f,
        },
        {translation.x, translation.y, translation.z, 1.0f} };
}

glm::mat3 TransformComponent::normalMatrix()
{
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);

    const glm::vec3 invScale = 1.f / scale;

    return glm::mat3{
        {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1),
        },
        {
            invScale.y * (c3 * s1 * s2 - c1 * s3),
            invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3),
        },
        {
            invScale.z * (c2 * s1),
            invScale.z * (-s2),
            invScale.z * (c1 * c2),
        }
    };
}

glm::mat4 GameObject::getTransformMat() 
{
    if (parentObject != nullptr) 
        return parentObject->getTransformMat() * transform.mat4();
    return transform.mat4(); 
}

glm::mat3 GameObject::getNormalMat()
{
    if (parentObject != nullptr) 
        return parentObject->getNormalMat() * transform.normalMatrix();
    return transform.normalMatrix();
}

void GameObjectModel::setModel(std::shared_ptr<Model> newModel)
{
    model = std::move(newModel);
    modelType = ModelType::OBJ_MODEL;
    hasModel = true;
}

void GameObjectModel::setModel(std::shared_ptr<GlTFModel::ModelGltf> newModel) {
    model = std::move(newModel);
    modelType = ModelType::GLTF_MODEL;
    hasModel = true;
}

void GameObjectModel::setModel(ModelVariant newModel)
{
    model = std::move(newModel);
    hasModel = true;
}

void GameObject::debugUI()
{
    ImGui::Text("Position:");
    ImGui::DragFloat3("##pos", glm::value_ptr(transform.translation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Rotation:");
    ImGui::DragFloat3("##rot", glm::value_ptr(transform.rotation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Scale:");
    ImGui::DragFloat3("##scl", glm::value_ptr(transform.scale), 0.01f, -10.0f, 10.0f);
}

std::string GameObject::getName() const
{
    if (name.empty())
        return "object_" + std::to_string(id);
    return name;
}

VkDescriptorImageInfo GameObjectModel::getTextureImageInfo() const
{
    if (!hasModel) return VkDescriptorImageInfo{};

    return std::visit([](const auto& modelInstance) -> VkDescriptorImageInfo {
        if (modelInstance) {
            return modelInstance->getTextureImageInfo();
        }
        return VkDescriptorImageInfo{};
    }, model);
}

void GameObjectModel::setMultipleInstances(std::vector<Model::Instance> instances)
{
    
    VkDeviceSize bufferSize = sizeof(Model::Instance) * instances.size();

    uint32_t instanceSize = sizeof(Model::Instance);

    instanceCount = static_cast<uint32_t>(instances.size());

    if (instanceCount == 0) return;

    Buffer stagingBuffer{ 
        device, 
        instanceSize, 
        instanceCount, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
    };

    stagingBuffer.map(); 
    stagingBuffer.writeToBuffer((void*)instances.data()); 

    instancesBuffer = std::make_unique<Buffer>(  
        device, 
        instanceSize, 
        instanceCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 
    );

    device.copyBuffer(stagingBuffer.getBuffer(), instancesBuffer->getBuffer(), bufferSize); 

}

void GameObjectModel::createDescriptorSet(DescriptorPool& pool) const
{
    if (!hasModel) return;

    std::visit([&pool, &device = this->device](const auto& modelInstance) {
        if (modelInstance) {
            modelInstance->createDescriptorSet(pool, device);
        }
        }, model);
}

std::vector<VkDescriptorSet> GameObjectModel::getDescriptorSets() const
{
    return std::visit([](const auto& modelInstance) -> std::vector<VkDescriptorSet> {
        if (modelInstance) {
            return modelInstance->getDescriptorSets(); 
        }
        return {};
        }, model);
}

void GameObjectModel::update(float dtime)
{
    if (animate) {
        animationTimer += dtime;

        std::visit([&](const auto& modelInstance) {
            if (modelInstance) {
                if (!modelInstance->updateAnimation(animationIndex, animationTimer)) {
                    animationTimer = 0.f;
                    animate = false;
                }
            }
            }, model);
    }
}

void GameObjectModel::bindModel(VkCommandBuffer& commandBuffer) const
{
    std::visit([&](const auto& modelInstance) {  
        if (modelInstance) {
            modelInstance->bind(commandBuffer, instancesBuffer.get()); 
        }
        }, model);
}

void GameObjectModel::drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint16_t frame_index, const std::array<FrustumPlane, 6>& frustrumPlanes)
{ 
    std::visit([&](const auto& modelInstance) {
        if (modelInstance) {
            modelInstance->draw(commandBuffer, pipelineLayout, frame_index, getTransformMat(), getNormalMat(), frustrumPlanes, instanceCount);
        }
        }, model);
}

void GameObjectModel::drawModelDepth(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, int cameraIndex, uint16_t frame_index, const std::array<FrustumPlane, 6>& planes)
{
    std::visit([&](const auto& modelInstance) { 
        if (modelInstance) {  
            modelInstance->drawDepth(commandBuffer, pipelineLayout, frame_index, getTransformMat(), cameraIndex, planes, instanceCount);
        }
        }, model); 
}


void GameObjectSpotLight::updateCameraView() { camera->setViewYXZ(transform.translation, transform.rotation); }

void GameObjectSpotLight::debugUI()
{
    ImGui::Text("Position:");
    ImGui::DragFloat3("##pos", glm::value_ptr(transform.translation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Rotation:");
    ImGui::DragFloat3("##rot", glm::value_ptr(transform.rotation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("fov");
    bool recreateMat = ImGui::DragFloat("##fov", &_fov, 0.01f, 0.1f, glm::half_pi<float>());

    ImGui::Text("Aspect Ratio");
    recreateMat = recreateMat || ImGui::DragFloat("##aspectRatio", &_aspect_ratio, 0.01f, 0.01f, 20.f); 

    if (recreateMat) camera->setPerspectiveProjection(_fov, _aspect_ratio, _nearClip, _farClip);

    ImGui::Text("Color:"); 
    ImGui::ColorEdit4("##clr", glm::value_ptr(transform.color)); 
     

}

void GameObjectCamera::debugUI()
{
    ImGui::Text("Position:"); 
    ImGui::DragFloat3("##pos", glm::value_ptr(transform.translation), 0.01f, -10.0f, 10.0f);  

    ImGui::Text("Rotation:"); 
    ImGui::DragFloat3("##rot", glm::value_ptr(transform.rotation), 0.01f, -10.0f, 10.0f); 

    ImGui::Text("fov");
    if (ImGui::DragFloat("##fov", &_fov, 0.01f, 0.1f, glm::half_pi<float>())) { 
        camera->setPerspectiveProjection(_fov, _aspect_ratio, _nearClip, _farClip); 
    }
}

void GameObjectCamera::updateCameraView() { camera->setViewYXZ(transform.translation, transform.rotation); camera->updateFrustrumPlanes(); }

SpotLight GameObjectSpotLight::getSpotLightInfo(bool _updateCameraView)
{
    if (_updateCameraView) updateCameraView(); 

    return {
        glm::vec4(transform.translation, 1.0), 
        transform.color, 
        glm::vec4(transform.rotation, 1.0), 
        camera->getProjection() * camera->getView() 
    }; 
}

void GameObjectModel::debugUI()
{
    ImGui::Text("Position:"); 
    ImGui::DragFloat3("##pos", glm::value_ptr(transform.translation), 0.01f, -10.0f, 10.0f); 

    ImGui::Text("Rotation:"); 
    ImGui::DragFloat3("##rot", glm::value_ptr(transform.rotation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Scale:");
    ImGui::DragFloat3("##scl", glm::value_ptr(transform.scale), 0.01f, -10.0f, 10.0f);

    ImGui::Text("animation:");
    ImGui::InputInt("index", &animationIndex);

    if (ImGui::Button("start"))
    {
        animate = true;
    }

	ImGui::Checkbox("show", &show);
}

void GameObjectPointLight::debugUI()
{
    ImGui::Text("Position:");
    ImGui::DragFloat3("##pos", glm::value_ptr(transform.translation), 0.01f, -10.0f, 10.0f);

    ImGui::Text("Color:"); 
    ImGui::ColorEdit3("##clr", glm::value_ptr(transform.color)); 

    ImGui::Text("intensity:");
    ImGui::DragFloat("##intensity", &transform.color.w, 0.1f, 0, 100);
}
