#pragma once

#include "Device.h"
#include "Window.h"
#include "objectManager.h"
#include "GameObject.h"

#define ENABLE_IMGUI

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <glm/glm.hpp>

class BasicUI
{
public:
	BasicUI(Device& device, GLFWwindow* window, VkRenderPass renderPass);
	~BasicUI();

	void drawUI(VkCommandBuffer commandBuffer, ObjectManager* manager);

	bool isWindowSelected = false;
private:

	void gameObjectWindow(GameObject* gameObject);
	void objectSelectionWindow(std::vector<std::string> listObjectsName, ObjectManager* manager);
	void createObjWindow(); 
	void createGLTFWindow();

	VkDescriptorPool imguiPool;
	Device& device;

	std::string selected_object = "";
};

#endif // ENABLE_IMGUI