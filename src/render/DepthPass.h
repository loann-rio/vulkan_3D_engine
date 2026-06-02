#pragma once

#include "BaseRenderPass.h"

class Device;
class AssetManager;

class DepthPass : BaseRenderPass
{
public:
	DepthPass(Device& device_, AssetManager& assets_)
		: BaseRenderPass(device_, assets_) {}

	void createRenderSystems() {};
	void recordPass() {};

private:
	void createRenderPass();
};