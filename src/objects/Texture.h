#pragma once

#include "../base/Device.h"

#include "../../external/basisu/transcoder/basisu_transcoder.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <fstream>

#include <glm/glm.hpp>

#include <ktx.h>
#include <ktxvulkan.h> 


class Texture
{
public:
	static std::unique_ptr<Texture> create(Device& device, const char* path, bool isCubeMap = false);
	static std::unique_ptr<Texture> create(Device& device, std::vector<std::vector<glm::vec2>> imageArray);

	static std::unique_ptr<Texture> createEmpty(Device& device, uint32_t width, uint32_t height, VkFormat format, bool isCubeMap);
	static std::unique_ptr<Texture> createEmpty(Device& device, VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount = 1);

	Texture(Device& device, const char* filePathTexture, bool isCubeMap);
	Texture(Device& device, unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize = 0, uint32_t mipLevel = 1);
	Texture(Device& device, VkImageView textureImageView) : device { device }, textureImageView { textureImageView } { isLoaded = true; }
	Texture(Device& device, uint32_t width, uint32_t height, VkFormat format, bool isCubeMap);
	Texture(Device& device, VkImageCreateInfo imageInfo, VkImageViewCreateInfo viewInfo, VkSamplerCreateInfo samplerInfo, VkImageLayout initImageLayout, uint32_t layerCount = 1);

	~Texture() {
		vkDestroySampler(device.device(), textureSampler, nullptr);
		vkDestroyImageView(device.device(), textureImageView, nullptr);
		vkDestroyImage(device.device(), textureImage, nullptr);
		vkFreeMemory(device.device(), textureImageMemory, nullptr);
	}

	VkDescriptorImageInfo getImageInfo() const;
	VkImageView getImageView() const { return textureImageView; }
	VkSampler getSampler() const { return textureSampler; }

	VkImage getImage() const { return textureImage; }

	bool isLoaded = false; 

private:

	bool createTextureImage(const char* path);
	bool createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imSize = 0);

	bool createTextureImage(float* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imSize = 0);

	bool createTextureImageKtx2(const std::string path, bool isCubeMap);
	bool createTextureImageKtx(const std::string path, bool isCubeMap);

	void generateMipChain(VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height);

	VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipmapLevel, VkImageAspectFlagBits aspectFlag, bool isCubeMap);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayer = 1, VkImageCreateFlags flags = 0);

	void createTextureSampler(VkSamplerAddressMode uvMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void bind(VkImage& image, VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory);
	
	VkImageView createTextureCubeMapImageView();

	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	
	Device& device;

	uint32_t mipLevel = 1;


};

