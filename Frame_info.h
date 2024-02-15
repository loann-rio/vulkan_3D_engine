#pragma once

#include "Camera.h"

#include <vulkan/vulkan.h>

class Frame_info
{
public:
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
	};
};

