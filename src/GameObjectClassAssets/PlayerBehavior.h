#pragma once

#include "../base/Device.h"
#include "../assetManager/AssetManager.h"


class PlayerBehavior : public GameObjectBehavior
{
public:
	REGISTER_BEHAVIOR(PlayerBehavior);

	PlayerBehavior(Device& device) : device{ device } {}

	void setup(Device& device, AssetManager& assetManager, GameObject& object) override {}
	void loop(Device& device, AssetManager& assetManager, GameObject& object) override {}

private:



	Device& device;
};