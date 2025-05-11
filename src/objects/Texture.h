#pragma once

#include "../base/Device.h"

#include "../../external/basisu/transcoder/basisu_transcoder.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <fstream>


class Texture
{
public:
	static std::unique_ptr<Texture> create(Device& device, const char* path) {
		auto tex = std::unique_ptr<Texture>(new Texture(device,  path, false)); 
		if (!tex->isLoaded) {
			return nullptr;
		}
		return tex;
	}

	static std::unique_ptr<Texture> createCubeMap(Device& device, const char* path) {
		auto tex = std::unique_ptr<Texture>(new Texture(device, path, true)); 
		if (!tex->isLoaded) {
			return nullptr;
		}
		return tex; 
	}

	Texture(Device& device, const char* filePathTexture, bool isCubeMap);
	
	Texture(Device& device, unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, VkDeviceSize imageSize = 0, uint32_t mipLevel = 1);
	Texture(Device& device, VkImageView textureImageView) : device { device }, textureImageView { textureImageView } {}
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

	void setSampler(VkSamplerCreateInfo info);

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	bool isLoaded = false; 

private:

	bool createTextureImage(const char* path);
	bool createTextureImageKtx2(const std::string path, bool isCubeMap);
	bool createTextureImageKtx(const std::string path, bool isCubeMap);

	void generateMipChain(VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayer = 1, VkImageCreateFlags flags = 0);

	void createTextureImageView();

	void createTextureSampler();

	VkImageView createTextureCubeMapImageView();

	void createTextureImage(unsigned char* rgbaPixels, const uint32_t fontWidth, const uint32_t fontHeight, uint32_t miplevel, VkDeviceSize imSize = 0);

	void bind(VkImage& image, VkMemoryPropertyFlags properties, VkDeviceMemory& imageMemory);
	
	VkImageView createImageView(VkImage image, VkFormat format);
	
	VkImageView textureImageView;
	VkSampler textureSampler;
	
	Device& device;

	uint32_t mipLevel = 1;


};

