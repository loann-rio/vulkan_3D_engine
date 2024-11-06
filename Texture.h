#pragma once

#include "Device.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <string>



class Texture
{
public:

	Texture(Device& device, const char* filePathTexture);
	Texture(Device& device, unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize = 0, uint32_t mipLevel = 1);
	Texture(Device& device) : device{ device } {}
	
	~Texture() {
		vkDestroySampler(device.device(), textureSampler, nullptr);
		vkDestroyImageView(device.device(), textureImageView, nullptr);
		vkDestroyImage(device.device(), textureImage, nullptr);
		vkFreeMemory(device.device(), textureImageMemory, nullptr);
	}

	VkDescriptorImageInfo getImageInfo();

	VkImageView getImageView() const { return textureImageView; }
	VkSampler getSampler() const { return textureSampler; }
	
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayou, uint32_t mipLevel = 1);

	void generateMipChain(VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels = 1);

	void createTextureImageView(uint32_t mipLevel = 1);
	void createTextureSampler(uint32_t mipLevel = 1);

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

private:

	uint32_t createTextureImage(const char* path);
	void createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imSize = 0, uint32_t mipLevel = 1);

	void bind(VkImage& image, VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory);
	
	VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipLevel = 1);
	
	VkImageView textureImageView;
	VkSampler textureSampler;
	
	Device& device;


};

