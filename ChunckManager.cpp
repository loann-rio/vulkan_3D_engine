#include "ChunckManager.h"

ChunckManager::ChunckManager(Device& device) : device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 16) 
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 16)
		.build();
}

void ChunckManager::updateChuncks(int x, int z)
{

	for (auto it = chunks.begin(); it != chunks.end(); ) {
		auto& chunk = it->second;

		if (abs(chunk.transform.translation.x/sizePlaneX - x) > radius ||
			abs(chunk.transform.translation.z/sizePlaneZ - z) > radius) {
			it = chunks.erase(it); 
		}
		else {
			++it; 
		}
	}


	for (int i = x - 1 ; i <= x ; i++) 	{
		for (int j = z - 1; j <= z; j++) {
			bool exist = false;
			for (auto& kv : chunks)	{
				auto& chunk = kv.second;

				if (chunk.transform.translation.x/sizePlaneX == i && chunk.transform.translation.z/sizePlaneZ == j)	{
					exist = true;
					break;
				}
			}

			if (!exist) {
				createChunk(i, j);
			}
		}
	}
}

void ChunckManager::createChunk(int x, int z)
{

	GenerateTerrainTile tileGenerator;

	tileGenerator.detailX = 200;
	tileGenerator.detailZ = 200;
	
	tileGenerator.x = 0;
	tileGenerator.z = 0;

	tileGenerator.offsetX = (float) x * (float) tileGenerator.detailX ;
	tileGenerator.offsetZ = (float) z * (float) tileGenerator.detailZ ;
	 
	tileGenerator.scale = 50.f;

	std::shared_ptr<Model> terrain = tileGenerator.generateMesh(device);

	tileGenerator.generateHeightMap(device);
	terrain->texture = tileGenerator.noiseTexture;

	auto terrain_object = GameObject::createGameObject(device);
	terrain_object.transform.translation = { x * sizePlaneX, 0.f, z * sizePlaneZ };
	terrain_object.model = terrain;
	chunks.emplace(terrain_object.getId(), std::move(terrain_object));

	terrain->createDescriptorSet(*descriptorPool, device);
}
