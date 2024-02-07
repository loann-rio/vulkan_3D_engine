#include "Window.h"

#include <stdexcept>

Window::Window(int w, int h, std::string n) : width{w}, height{h}, windowName{n} {
	initWindow();
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::initWindow()
{
	glfwInit();

	// desable openGL api as we use Vulkan
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// desable resizable window
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// create window
	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);


}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}
