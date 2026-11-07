#pragma once

#include "../objects/objectManager.h"

class Test : public GameObjectBehavior {
	void setup(Device& device, AssetManager& AssetManager, GameObject& object) override {}
	void loop(Device& device, AssetManager& AssetManager, GameObject& object) override {}
};
