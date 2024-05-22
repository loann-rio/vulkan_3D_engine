#pragma once

#include "Window.h"
#include "device.h"
#include "GameObject.h"
#include "Renderer.h"
#include "descriptors.h"
#include "TextOverlay.h"

#include <memory>
#include <vector>
#include <deque>

class App
{
public:
	static constexpr int WIDTH = 1600;
	static constexpr int HEIGHT = 1200;

	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;

	void run();

private:
	void loadGameObjects();
	void getFrameRate(float lastFrameTime);

	Window window{ WIDTH, HEIGHT, "hello" };
	Device device{ window };
	Renderer renderer{ window, device };

	std::unique_ptr<DescriptorPool> globalPool{};
	GameObject::Map gameObjects;

	std::vector<float> frameTimeVector;
	float frameTimeSum = 0;
};

