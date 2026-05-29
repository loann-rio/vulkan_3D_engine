#pragma once

#include "base/Window.h"
#include "base/device.h"
#include "base/descriptors.h"

#include "render/Renderer.h"
#include "objects/ObjectManager.h"

#include "assetManager/AssetManager.h"

#include <memory>


class Engine
{
public:
	static constexpr int WIDTH = 1600;
	static constexpr int HEIGHT = 1200;

	Engine();
	~Engine() {};

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();

private:

	Window window{ WIDTH, HEIGHT, "vulkan engine" };
	Device device{ window };
	AssetManager assetManager{};
	ObjectManager objectManager{ device, assetManager };
	Renderer renderer{ window, device, assetManager, objectManager };
};
