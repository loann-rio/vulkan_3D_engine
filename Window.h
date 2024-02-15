#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

class Window
{
private:

	void initWindow();
	static void frameBufferResizeCallBack(GLFWwindow *window, int width, int height);

	int width;
	int height;
	bool frameBufferResized = false;

	std::string windowName;
	GLFWwindow* window;

public:
	Window(int w, int h, std::string n);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool shouldClose() { return glfwWindowShouldClose(window); }
	VkExtent2D getExtent() { return { static_cast<uint32_t>(width),  static_cast<uint32_t>(height) }; }

	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	bool wasWindowResized() { return frameBufferResized; }
	void resetWindowResizedFlag() { frameBufferResized = false; }
	GLFWwindow* getGLFWwindow() const { return window; }
	

};

