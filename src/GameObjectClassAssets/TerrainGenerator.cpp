#include "TerrainGenerator.h"

#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

#include <array>

void TerrainGenerator::loop(Device& device, ObjectManager* objManager, GameObject* object)
{
	int posX = object->transform.translation.x / chunkWorldSide;
	int posY = object->transform.translation.z / chunkWorldSide;

	//int posX = 0;
	//int posY = 0;

	AssetManager& assets = objManager->assetManager;
	
	
	for (int i = posX - int(sizeWorldInChunck / 2); i <= posX + int(sizeWorldInChunck / 2); i++)
	for (int j = posY - int(sizeWorldInChunck / 2); j <= posY + int(sizeWorldInChunck / 2); j++)
	{
		int hash = i * 5463 + j * 9875;

		if (loadedChunk.count(hash) == 0) {
			std::cout << "chunk not found, creating chunk \n";
			
			// push terrain
			auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, objManager->assetManager);
			gameObject->transform.translation = { i * chunkWorldSide, 4, j * chunkWorldSide };
			gameObject->setModelSubType(ModelSubType::TERRAIN);
			//gameObject->setParent(objManager->get("terrain G"));
			gameObject->saveable = false;

			GameObject::id_t id_terrain = gameObject->getId();
			objManager->pushGameObject(std::move(gameObject));
			loadedChunk[hash] = id_terrain;

			// create terrain and place trees
			objManager->pushFuture(std::async(std::launch::async, [this, &assets, id_terrain, i, j]() {

				// create height map from noise function
				std::vector<std::vector<glm::vec2>> heightMap = generateChunck(i * (this->chunkSize - 1), j * (this->chunkSize - 1));
				
				// separate height and slope
				std::vector<std::vector<float>> map = std::vector<std::vector<float>>(this->chunkSize, std::vector<float>(this->chunkSize));
				for (int x = 0; x < this->chunkSize; x++) for (int y = 0; y < this->chunkSize; y++) map[y][x] = heightMap[x][y].x;

				// create plane object
				std::shared_ptr<Model> plane = PrebuiltModel::createTerrain(this->device, assets, 4, 4, map);
				



				std::vector<std::vector<std::vector<float>>> heightMapVector = std::vector<std::vector<std::vector<float>>>(this->chunkSize, std::vector<std::vector<float>>(this->chunkSize, std::vector<float>(2)));
				for (int x = 0; x < this->chunkSize; x++) 
				for (int y = 0; y < this->chunkSize; y++) 
				{
					heightMapVector[y][x][0] = heightMap[x][y].x;
					heightMapVector[y][x][1] = heightMap[x][y].y;
				}
				
				TextureBuilder builder(this->device);
				std::unique_ptr<TextureObject> text = builder.fromVector(heightMapVector).build();

				plane->setTexture(std::move(text));
					
				return std::vector<futureObject>{futureObject{ plane, plane ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id_terrain, {}, false }};
				}));
		}
	}

	
	std::unordered_set<int> validHashes;
	validHashes.reserve(sizeWorldInChunck * sizeWorldInChunck);

	for (int i = posX - int(sizeWorldInChunck / 2 + 1); i <= posX + int(sizeWorldInChunck / 2 + 1); i++) {
		for (int j = posY - int(sizeWorldInChunck / 2 + 1); j <= posY + int(sizeWorldInChunck / 2 + 1); j++) {
			int hash = i * 5463 + j * 9875;
			validHashes.insert(hash);
		}
	}

	// Iterate loaded chunks and remove invalid ones
	for (auto it = loadedChunk.begin(); it != loadedChunk.end();) {
		if (validHashes.find(it->first) == validHashes.end()) { // id is not in valid chunk, remove it
			objManager->removeGameObject(it->second);
			it = loadedChunk.erase(it);

			std::cout << "unload chunck \n";
		}
		else {
			++it;
		}
	}

}

TerrainGenerator::TerrainGenerator(Device& device) : device(device)
{

	if (globalScale <= 0) {
		globalScale = 0.0001f;
	}
}


std::vector<std::vector<glm::vec2>> TerrainGenerator::generateChunck(float Xoffset, float Yoffset) {
	std::mt19937 prng(seed);

	std::vector<glm::vec2> octavesOffsets(maxOctaves);
	for (int i = 0; i < maxOctaves; i++) {
		float offsetX = Xoffset + prng() % 200000 - 100000;
		float offsetY = Yoffset + prng() % 200000 - 100000;
		octavesOffsets[i] = { offsetX, offsetY };
	}

	std::vector<std::vector<glm::vec2>> noiseMap(chunkSize, std::vector<glm::vec2>(chunkSize));

	for (int y = 0; y < chunkSize; y++) {
		for (int x = 0; x < chunkSize; x++) {

			float amplitude = 1;
			float frequency = 1;
			float noiseHeight = 0;
			float m = 0;
			float slope = 0;

			std::vector<glm::vec2> lookupVoronoi = VoronoiNoise::voronoi({ (int)((x + Xoffset)), (int)((y + Yoffset)) }, biomeScale, 1);
			std::vector<glm::vec2> lookupVoronoi1 = VoronoiNoise::voronoi({ (int)((x + 1 + Xoffset)), (int)((y + Yoffset)) }, biomeScale, 1);
			std::vector<glm::vec2> lookupVoronoi2 = VoronoiNoise::voronoi({ (int)((x - 1 + Xoffset)), (int)((y + Yoffset)) }, biomeScale, 1);
			std::vector<glm::vec2> lookupVoronoi3 = VoronoiNoise::voronoi({ (int)((x + Xoffset)), (int)((y + 1 + Yoffset)) }, biomeScale, 1);
			std::vector<glm::vec2> lookupVoronoi4 = VoronoiNoise::voronoi({ (int)((x + Xoffset)), (int)((y - 1 + Yoffset)) }, biomeScale, 1);

			for (int i = 0; i < maxOctaves; i++) {
				float sampleX = ((float)x + (float)octavesOffsets[i].x) / globalScale * frequency;
				float sampleY = ((float)y + (float)octavesOffsets[i].y) / globalScale * frequency;

				// position + 1 to calculate derivative
				float sampleXp1 = ((float)x + 1 + (float)octavesOffsets[i].x) / globalScale * frequency;
				float sampleYp1 = ((float)y + 1 + (float)octavesOffsets[i].y) / globalScale * frequency;
				
				float perlinValue = (pn.noise(sampleX, sampleY) * 2 - 1);

				// get slope at point
				float perlinValueXp1 = (pn.noise(sampleXp1, sampleY) * 2 - 1) - perlinValue;
				float perlinValueYp1 = (pn.noise(sampleX, sampleYp1) * 2 - 1) - perlinValue;

				m += (pow(perlinValueXp1, 2) + pow(perlinValueYp1, 2)) * amplitude;

				slope += sqrt(perlinValueXp1 * perlinValueXp1 + perlinValueYp1 * perlinValueYp1) * amplitude;

				noiseHeight += (perlinValue + .5f) * amplitude / (1 + m * weightedRegionValue(lookupVoronoi, &RegionVariables::gradientFactor));
				
				float amplitude1 = weightedRegionValue(lookupVoronoi, &RegionVariables::persistance);  // persistance : [0, 1]
				amplitude1 += weightedRegionValue(lookupVoronoi1, &RegionVariables::persistance);  // persistance : [0, 1]
				amplitude1 += weightedRegionValue(lookupVoronoi2, &RegionVariables::persistance);  // persistance : [0, 1]
				amplitude1 += weightedRegionValue(lookupVoronoi3, &RegionVariables::persistance);  // persistance : [0, 1]
				amplitude1 += weightedRegionValue(lookupVoronoi4, &RegionVariables::persistance);  // persistance : [0, 1]

				amplitude *= (amplitude1 / 5.f);  // average persistance

				float frequency1 = weightedRegionValue(lookupVoronoi, &RegionVariables::lacunarity);  // lacunarity : > 1
				frequency1 += weightedRegionValue(lookupVoronoi1, &RegionVariables::lacunarity);  // lacunarity : > 1
				frequency1 += weightedRegionValue(lookupVoronoi2, &RegionVariables::lacunarity);  // lacunarity : > 1
				frequency1 += weightedRegionValue(lookupVoronoi3, &RegionVariables::lacunarity);  // lacunarity : > 1
				frequency1 += weightedRegionValue(lookupVoronoi4, &RegionVariables::lacunarity);  // lacunarity : > 1
				frequency *= (frequency1 / 5.f);  // average lacunarity
				//frequency *= weightedRegionValue(lookupVoronoi, &RegionVariables::lacunarity);
			}

			noiseMap[x][y] = glm::vec2(globalHeightMultiplier * weightedRegionValue(lookupVoronoi, &RegionVariables::heightMultiplier) * noiseHeight, slope);
		}
	}

	return noiseMap;
}

std::vector<Model::Instance> TerrainGenerator::placeTrees(std::vector<std::vector<glm::vec2>> heightMap, float Xoffset, float Yoffset) const
{
	std::vector<Model::Instance> treeList = {};

	srand(Xoffset + Yoffset);

	for (int x = 0; x < chunkSize; x++)
	for (int y = 0; y < chunkSize; y++) 
	{

		if (rand() % 100 < treeProbability) { 

			float sizeFactor = (0.75 + (float(rand() % 100) / 200.f));

			float yPos = 4.0 * ((y - 0.5 + (float(rand() % 100) / 100.f)) / chunkSize);
			float xPos = 4.0 * ((x - 0.5 + (float(rand() % 100) / 100.f)) / chunkSize);

			if (abs(heightMap[x][y].y) < 0.02 && heightMap[x][y].x < 2.f)
			{
				Model::Instance instance = {
					{ Xoffset + xPos, -heightMap[x][y].x, Yoffset + yPos},
					{0, 0, 0},
					{0.1,  -0.1 * sizeFactor, 0.1} 
				};

				treeList.push_back(instance);
			}
		}
	}

	return treeList;
}

std::vector<std::vector<glm::vec3>> TerrainGenerator::generatecolorMap(std::vector<std::vector<glm::vec2>> heightMap)
{
	std::vector<std::vector<glm::vec3>> colorMap(chunkSize, std::vector<glm::vec3>(chunkSize)); 

	for (int y = 0; y < chunkSize; y++) 
	{
		for (int x = 0; x < chunkSize; x++) 
		{

			float height = -1 * heightMap[x][y].x;
			float slope = abs(heightMap[x][y].y);

			glm::vec3 color = { 0, 0, 0 };

			// larger abs(y) -> min slope
			if (slope > 0.7f) {
				// flat terrain

				if (height < 2.3f)
				{
					// grass
					color = { 0.1, 0.7, 0.2 };
				}
				else
				{
					// dirt
					color = { 0.2, 0.5, 0.1 };
				}

				if (height < 2.5f && slope < 0.8f)
				{
					// grass
					color = { 0.2, 0.7, 0.2 };
				}
				else if (height < 2.7f && slope < 0.8f)
				{
					// dirt
					color = { 0.2, 0.5, 0.2 };
				}
				else if (height > 2.5)
				{
					// snow
					color = { 0.78f, 0.78f, 0.9f };
				}
			}
			else
			{
				// rock cliff
				color = { 0.25f, 0.239f, 0.219f };

				// dirt
				// modelBuilder.vertices[i].color = { 0.384f, 0.274f, 0.106f };
			}

			colorMap[x][y] = color;
		}
	}

	return colorMap;
}

float TerrainGenerator::weightedRegionValue(const std::vector<glm::vec2>& lookupVoronoi, float RegionVariables::* member)
{
	float r = 0;
	float sum = 0;

	for (glm::vec2 val : lookupVoronoi) {
		const RegionVariables& reg = regions[int(val.y) % regions.size()];
		r += reg.*member * val.x;
		sum += val.x;
	}

	return r / sum;
}
