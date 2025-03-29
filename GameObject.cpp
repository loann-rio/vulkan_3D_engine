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

std::unique_ptr<GameObject> GameObject::makePointLight(Device& device, float intencity, float radius, glm::vec3 color = glm::vec3{ 1.f })
{
    /*auto gameObj = GameObject::createGameObject(device);
    //gameObj.color = color;
    gameObj->transform.color = glm::vec4(color, 1.f);
    gameObj->transform.scale.x = radius;
    gameObj->pointLight = std::make_unique<PointLightComponent>();
    gameObj->pointLight->LightIntencity = intencity;
    return gameObj;*/
    return nullptr;
}

void GameObjectModel::setModel(ModelVariant newModel)
{
    model = std::move(newModel);
    hasModel = true;
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

void GameObjectModel::bindModel(VkCommandBuffer& commandBuffer) const
{
    std::visit([&](const auto& modelInstance) {
        if (modelInstance) {
            modelInstance->bind(commandBuffer);
        }
        }, model);
}

void GameObjectModel::drawModel(VkCommandBuffer& commandBuffer, VkPipelineLayout& GlTFPipelineLayout) const
{
    std::visit([&](const auto& modelInstance) {
        if (modelInstance) {
            modelInstance->draw(commandBuffer, GlTFPipelineLayout);
        }
        }, model);
}

void GameObjectSpotLight::updateCameraView() { camera->setViewYXZ(transform.translation, transform.rotation); }

void GameObjectCamera::updateCameraView() { camera->setViewYXZ(transform.translation, transform.rotation); }

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


