#include "ChunckManager.h"

ChunckManager::ChunckManager(Device& device) : device{ device }
{
	descriptorPool = DescriptorPool::Builder(device)
		.setMaxSets(Swap_chain::MAX_FRAMES_IN_FLIGHT * 16) 
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swap_chain::MAX_FRAMES_IN_FLIGHT * 16)
		.build();

}

void ChunckManager::updateChuncks(int x, int y)
{

	for (auto it = chunks.begin(); it != chunks.end(); ) 
	{
		auto& chunk = it->second;

		if (abs(chunk.transform.translation.x/sizePlaneX - x) > radius ||
			abs(chunk.transform.translation.z/sizePlaneY - y) > radius) {
			it = chunks.erase(it); 

			std::cout << "remove chunk \n";
		}
		else 
		{
			std::cout << "&\n";
			++it; 
		}
	}


	std::cout << " rad : " << x << " " << radius / 2 << " " << x - radius / 2 << " " << x + radius / 2 << "\n";
	for (int i = x - radius / 2; i <= x + radius / 2; i++) 
	{
		for (int j = y - radius / 2; j <= y + radius / 2; j++) 
		{
			std::cout << i << " " << j << "\n";

			bool exist = false;
			for (auto& kv : chunks)
			{
				auto& chunk = kv.second;

				if (chunk.transform.translation.x/sizePlaneX == i && chunk.transform.translation.z/sizePlaneY == j)
				{
					std::cout << "exist" << "\n";;
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
	std::cout << "create chunk\n";
	GenerateTerrainTile tileGenerator;
	std::shared_ptr<Model> terrain = tileGenerator.generateMesh(device);

	tileGenerator.offsetX = (float) x * (float) tileGenerator.detailX / tileGenerator.scale;
	tileGenerator.offsetY = (float) z * (float) tileGenerator.detailY / tileGenerator.scale;

	tileGenerator.generateHeightMap(device);
	terrain->texture = tileGenerator.noiseTexture;

	auto terrain_object = GameObject::createGameObject(device);
	terrain_object.transform.translation = { x * sizePlaneX, 0.f, z * sizePlaneY };
	terrain_object.model = terrain;
	chunks.emplace(terrain_object.getId(), std::move(terrain_object));

	terrain->createDescriptorSet(*descriptorPool, device);


}
