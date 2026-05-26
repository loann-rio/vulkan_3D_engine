#pragma once

#include "../base/Device.h"
#include "../base/Window.h"

#include "../objects/objectManager.h"
#include "../objects/GameObject.h"
#include "../base/Frame_info.h"

#define ENABLE_IMGUI

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <glm/glm.hpp>

#define _CRT_SECURE_NO_WARNINGS 
#include <nfd.h>

class BasicUI
{
public:
	BasicUI(Device& device, AssetManager& assets, GLFWwindow* window, VkRenderPass renderPass);
	~BasicUI();

	void drawUI(VkCommandBuffer commandBuffer, ObjectManager* manager, float fps);

	bool isWindowSelected = false; 
private:

	void gameObjectWindow(GameObject* gameObject, ObjectManager* manager); 
	void objectSelectionWindow(std::vector<std::string> listObjectsName, ObjectManager* manager, float fps);
	void terrainUboWindow(TerrainUbo& terrainUbo);

	void createObjWindow(ObjectManager* manager);
	void createGLTFWindow(ObjectManager* manager);
	void createSkyboxWindow(ObjectManager* manager);
	void createCameraWindow(ObjectManager* manager, bool isSpotLight);
	void createEmptyObjectWindow(ObjectManager* manager);

	std::string openFileDialog(const char* filter);

	VkDescriptorPool imguiPool; 
	Device& device;
	AssetManager& assets;

	std::string selected_object = "";

	bool show_create_go_window = true; 
	bool show_create_terrain_window = true;
	int selected = -1;
};

#endif // ENABLE_IMGUI