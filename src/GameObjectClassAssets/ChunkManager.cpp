#include "ChunkManager.h"

void ChunkManager::loop(Device& device, ObjectManager* objManager, GameObject* object)
{

	// determine current chunk position
	int posX = object->transform.translation.x / chunkWorldSize;
	int posY = object->transform.translation.z / chunkWorldSize;

	// load visible chunks
	for (int i = posX - int(visibleChunk / 2); i <= posX + int(visibleChunk / 2); i++)
	for (int j = posY - int(visibleChunk / 2); j <= posY + int(visibleChunk / 2); j++)
	{
		if (abs(i) > int(sizeWorldInChunck / 2)) continue; // if outisde of the max world size, skip
		if (abs(j) > int(sizeWorldInChunck / 2)) continue; // if outisde of the max world size, skip
		int hash = hashFunction(i, j);

		if (loadedChunk.count(hash) > 0) continue; // if chunk already exist -> skip

		// create chunk and store its object ids
		std::vector<GameObject::id_t> chunkObjects = terrainGenerator.createChunk(device, objManager, i, j, chunkWorldSize);
		loadedChunk[hash] = chunkObjects;
	}

	// unload non-valid chunk
	std::unordered_set<int> validHashes;
	validHashes.reserve(sizeWorldInChunck * sizeWorldInChunck);

	for (int i = posX - int(visibleChunk / 2 + 1); i <= posX + int(visibleChunk / 2 + 1); i++) {
		for (int j = posY - int(visibleChunk / 2 + 1); j <= posY + int(visibleChunk / 2 + 1); j++) {
			int hash = hashFunction(i, j);
			validHashes.insert(hash);
		}
	}

	// Iterate loaded chunks and remove invalid ones
	for (auto it = loadedChunk.begin(); it != loadedChunk.end();) {
		
		if (validHashes.find(it->first) == validHashes.end()) { // id is not in valid chunk, remove it
			// remove chunk with all its component
			
			for (auto id : it->second) {
				objManager->removeGameObject(id);
			}

			it = loadedChunk.erase(it);
		}
		else {
			++it;
		}
	}
}	


int ChunkManager::hashFunction(int x, int y) const {
	return x * 5463 + y * 9875;
}
