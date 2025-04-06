#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "Device.h"
#include "descriptors.h"


class IModel
{
public:
	virtual std::unique_ptr<IModel> createModelFromFile(Device& device, const std::string& filePath, const char* filePathTexture) = 0;
	virtual void draw(VkCommandBuffer commandBuffer) = 0;
	virtual void update() = 0;
	virtual void updateAnimation() = 0;
	virtual void bind(VkCommandBuffer commandBuffer) = 0;
	virtual void createDescriptorSet(DescriptorPool& pool, Device& device) = 0;
	virtual std::vector<VkDescriptorSet> getDecriptorSet() = 0;
	//virtual std::unique_ptr<Texture> getTexture
	//std::unique_ptr<Texture> texture;
};