#include "GameObject.h"
#include <glm/gtc/type_ptr.hpp>

 
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


void GameObjectModel::setModel(ModelManager::ModelID _model)
{
    lodModelAssets.push_back(_model);
    hasModel = true;
}

void GameObjectModel::setModel(std::vector<ModelManager::ModelID> lod_models)
{
    lodModelAssets = lod_models;
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


void GameObjectModel::setMultipleInstances(std::vector<ModelInstance> instances)
{
    enableCpuCulling = false; // not managed for multiple instance, should be doe on gpu

    VkDeviceSize bufferSize = sizeof(ModelInstance) * instances.size();
    instanceCount = static_cast<uint32_t>(instances.size());
    if (instanceCount == 0) return;

    uint32_t instanceSize = sizeof(ModelInstance);

    Buffer stagingBuffer{ 
        device, 
        instanceSize, 
        instanceCount, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
    };

    stagingBuffer.map(); 
    stagingBuffer.writeToBuffer(instances.data()); 

    instancesBuffer = std::make_unique<Buffer>(  
        device, 
        instanceSize, 
        instanceCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 
    );

    device.copyBuffer(stagingBuffer.getBuffer(), instancesBuffer->getBuffer(), bufferSize); 

    frameInstancesBuffer.clear();
    frameInstancesBuffer.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < frameInstancesBuffer.size(); ++i) {
        frameInstancesBuffer[i] = std::make_unique<Buffer>(
            device,
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        device.copyBuffer(instancesBuffer->getBuffer(), frameInstancesBuffer[i]->getBuffer(), bufferSize);
    }
}

void GameObjectModel::createInstanceComputeDescriptorSets(DescriptorPool& pool)
{
    instanceComputeDescriptorSets.clear();

    // nothing to do if no instances or no buffers
    if (!instancesBuffer || frameInstancesBuffer.empty()) return;

    instanceComputeDescriptorSets.resize(frameInstancesBuffer.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < frameInstancesBuffer.size(); ++i) {
        VkDescriptorSet set = VK_NULL_HANDLE;
        Buffer* dstBuffer = frameInstancesBuffer[i].get();
        if (!dstBuffer) continue;

        auto setLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

        VkDescriptorSet computeSet = VK_NULL_HANDLE;
        if (!pool.allocateDescriptor(setLayout->getDescriptorSetLayout(), computeSet)) {
            std::cout << "ComputePass::createDescriptorSet: Failed to allocate descriptor set\n";
			throw std::runtime_error("failed to allocate descriptor set!");
        }

        auto srcInfo = instancesBuffer.get()->descriptorInfo();
        auto dstInfo = dstBuffer->descriptorInfo();

        DescriptorWriter(*setLayout, pool)
            .writeBuffer(0, &srcInfo)
            .writeBuffer(1, &dstInfo)
            .build(computeSet);


        instanceComputeDescriptorSets[i] = computeSet;
     
    }
}

VkDescriptorSet GameObjectModel::getInstanceComputeDescriptorSet(uint16_t frameIndex) const
{
    if (frameIndex >= instanceComputeDescriptorSets.size()) return VK_NULL_HANDLE;
    return instanceComputeDescriptorSets[frameIndex];
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


    if (ImGui::Button("start")) animator.play(static_cast<uint32_t>(std::max(0, animationIndex)), false);
    ImGui::SameLine();
    if (ImGui::Button("stop")) animator.stop();

	ImGui::Checkbox("show", &show);
}

void GameObjectModel::createPoseResources(DescriptorPool& pool)
{
    if (lodModelAssets.empty()) return;
    const ModelAsset* asset = assets.models().get(lodModelAssets[0]);
    if (!asset || asset->skeleton.empty()) return;

    animator.init(asset->skeleton);

    auto setLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    framePoseBuffers.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);
    poseDescriptorSets.resize(Swap_chain::MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < Swap_chain::MAX_FRAMES_IN_FLIGHT; ++i) {
        framePoseBuffers[i] = std::make_unique<Buffer>(
            device,
            sizeof(glm::mat4),
            asset->skeleton.matrixCount,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        framePoseBuffers[i]->map();

        auto info = framePoseBuffers[i]->descriptorInfo();
        DescriptorWriter(*setLayout, pool)
            .writeBuffer(0, &info)
            .build(poseDescriptorSets[i]);

        uploadPose(static_cast<uint16_t>(i));
    }
}

void GameObjectModel::uploadPose(uint16_t frameIndex)
{
    if (frameIndex >= framePoseBuffers.size()) return;
    const auto& matrices = animator.matrices();
    if (matrices.empty()) return;

    framePoseBuffers[frameIndex]->writeToBuffer((void*)matrices.data(), matrices.size() * sizeof(glm::mat4));
}

void GameObjectModel::update(float dtime)
{
    if (lodModelAssets.empty()) return;
    const ModelAsset* asset = assets.models().get(lodModelAssets[0]);
    if (asset) animator.update(dtime, asset->skeleton);
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
