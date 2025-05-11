#pragma once

#include "../base/Device.h"
#include "../base/Window.h"

#include "../objects/objectManager.h"
#include "../objects/GameObject.h"

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

	void gameObjectWindow(GameObject* gameObject, ObjectManager* manager); 
	void objectSelectionWindow(std::vector<std::string> listObjectsName, ObjectManager* manager);
	void createObjWindow(ObjectManager* manager);
	void createGLTFWindow(ObjectManager* manager);
	void createCameraWindow(ObjectManager* manager, bool isSpotLight);

	VkDescriptorPool imguiPool; 
	Device& device;

	std::string selected_object = "";

	bool show_create_go_window = true; 
	int selected = -1;
};

#endif // ENABLE_IMGUI