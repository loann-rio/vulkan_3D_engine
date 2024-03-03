#pragma once

#include "Device.h"
#include <vulkan/vulkan.h>

#include <string>



class Texture
{
public:

	Texture(Device& device);
	~Texture() {
		vkDestroySampler(device.device(), textureSampler, nullptr);
		vkDestroyImageView(device.device(), textureImageView, nullptr);
		vkDestroyImage(device.device(), textureImage, nullptr);
		vkFreeMemory(device.device(), textureImageMemory, nullptr);
	}

	void createTextureImage(const char* path);
	
	VkDescriptorImageInfo getImageInfo();

	void createTextureImageView();
	void createTextureSampler();
	

private:

	void bind(VkImage& image, VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayou);
	VkImageView createImageView(VkImage image, VkFormat format);
	
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	
	Device& device;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	VkSampler textureSampler;

	

};

