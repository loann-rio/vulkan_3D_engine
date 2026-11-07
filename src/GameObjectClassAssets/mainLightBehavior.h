#pragma once

#include "../objects/objectManager.h"

class mainLightBehavior : public GameObjectBehavior {
	void setup(Device& device, AssetManager& assetManager, GameObject& object) override {}


	void loop(Device& device, AssetManager& assetManager, GameObject& object) override {
		auto* camera = assetManager.gameObject().findByName("mainCamera");
		object.transform.translation.x = camera->transform.translation.x;
		object.transform.translation.z = camera->transform.translation.z;
	}
};
