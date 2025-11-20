#pragma once

#include "finiteSizedTerrainGeneration.h"

#include "../base/Device.h"
#include "../objects/objectManager.h"

#include <unordered_set>
#include <iostream>
#include <vector>

class ChunkManager : public GameObjectBehavior
{
public:
	REGISTER_BEHAVIOR(ChunkManager);

	ChunkManager(Device& device) : device{ device } {}

	void setup(Device& device, ObjectManager* objManager, GameObject* object) override {} 
	void loop(Device& device, ObjectManager* objManager, GameObject* object) override;

private:

	int hashFunction(int x, int y) const;

	// map using position hash -> vector of game object composing the chunk
	std::map<int, std::vector<GameObject::id_t>> loadedChunk; 

	const uint32_t visibleChunk = 5; // diametre of chunk visible around the player
	const uint16_t sizeWorldInChunck = 5; // number of chunk per side in the world
	const float chunkWorldSize = 1.f; // size of a chunk in world unit

	finiteSizedTerrainGeneration terrainGenerator;

	Device& device;
};