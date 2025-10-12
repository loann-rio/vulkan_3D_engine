#include "Window.h"

#include <stdexcept>
#include <fstream>
#include <../json.hpp>
using json = nlohmann::json;

Window::Window(int w, int h, std::string n) : width{w}, height{h}, windowName{n} {
	initWindow();

	if (window == nullptr) {
		throw std::runtime_error("failed to create window");
	}
}

Window::~Window()
{

	int xpos, ypos;
	glfwGetWindowPos(window, &xpos, &ypos); 

	std::ofstream out(windowConfigPath);
	if (out.is_open()) {
		json j;
		j["width"] = width;
		j["height"] = height;
		j["xpos"] = xpos;
		j["ypos"] = ypos;

		out << j.dump(4);
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::initWindow()
{
	//read window size from config file or create one
	json j;
	int xpos = 100;
	int ypos = 100;

	if (std::filesystem::exists(windowConfigPath)) {
		std::ifstream in(windowConfigPath);
		if (in.is_open()) {
			in >> j;
			width  = j["width"].get<int>();
			height = j["height"].get<int>();
			xpos   = j["xpos"].get<int>();
			ypos   = j["ypos"].get<int>();
		}
	}
	
	glfwInit();

	// desable openGL api as we use Vulkan
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// resizable window
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// create window
	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	glfwSetWindowPos(window, xpos, ypos);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, frameBufferResizeCallBack);
}

void Window::frameBufferResizeCallBack(GLFWwindow* window1, int width, int height)
{
	auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window1));
	window->frameBufferResized = true;
	window->width = width;
	window->height = height;
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}
