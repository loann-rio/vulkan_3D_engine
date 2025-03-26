#pragma once

#include "Device.h"
#include "Window.h"
#include "GameObject.h"

#define ENABLE_IMGUI
#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#endif // ENABLE_IMGUI

#include <glm/glm.hpp>


class BasicUI
{
public:
	BasicUI(Device& device, GLFWwindow* window, VkRenderPass renderPass);
	~BasicUI();

	void drawUI(VkCommandBuffer commandBuffer, GameObject* gameObject);
	void gameObjectWindow(GameObject* gameObject);

private:
	VkDescriptorPool imguiPool;
	Device& device;
};

