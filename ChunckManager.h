#pragma once

#include <iostream>
#include <vector>

#include "GameObject.h"
#include "GenerateTerrainTile.h"

class ChunckManager
{
public:
	ChunckManager(Device& device);

	int radius = 3;
	GameObject::Map chunks;

	const float sizePlaneX = 5;
	const float sizePlaneY = 5;

	void updateChuncks(int x, int y);
	void createChunk(int x, int z);

private:

	Device& device;
	std::unique_ptr<DescriptorPool> descriptorPool{};
};

