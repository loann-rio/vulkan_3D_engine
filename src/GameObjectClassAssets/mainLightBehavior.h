#pragma once

#include "../objects/objectManager.h"

class mainLightBehavior : public GameObjectBehavior {
	void setup(Device& device, ObjectManager* objManager, GameObject* object) override {}


	void loop(Device& device, ObjectManager* objManager, GameObject* object) override {
		auto* camera = objManager->get("mainCamera");
		object->transform.translation.x = camera->transform.translation.x;
		object->transform.translation.z = camera->transform.translation.z;
	}
};
