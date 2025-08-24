#pragma once

#include "../objects/objectManager.h"

class Test : public GameObjectBehavior {
	void setup(Device& device, ObjectManager* objManager, GameObject* object) override {}
	void loop(Device& device, ObjectManager* objManager, GameObject* object) override {}
};
