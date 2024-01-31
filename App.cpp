#include "App.h"

void App::run()
{
	while (!window.shouldClose())
	{
		glfwPollEvents();
	}
}