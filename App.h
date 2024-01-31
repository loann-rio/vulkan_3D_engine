#pragma once

#include "Window.h"
#include "Pipeline.h"
#include <string>

class App
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	void run();

private:
	Window window{ WIDTH, HEIGHT, "hello" };

	Pipeline pipeline{"simple_shader.vert.spv", "simple_shader.frag.spv"};
};

