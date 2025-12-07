#include "finiteSizedTerrainGeneration.h"

#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

// TO DO:
// type of terrain generation
// region
// height
// trees
// city
// roads
// accessorys

///////

// volcanic island:
// base start point
// volcano shape
// random spread

std::vector<GameObject::id_t> finiteSizedTerrainGeneration::createChunk(Device& device, ObjectManager* objManager, int chunkX, int chunkY, float chunkWorldSizeUnit)
{
	auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device, objManager->assetManager);
	gameObject->transform.translation = { chunkX * chunkWorldSizeUnit, 0, chunkY * chunkWorldSizeUnit };
	gameObject->setModelSubType(ModelSubType::TERRAIN);
	gameObject->saveable = false;

	GameObject::id_t id_terrain = gameObject->getId();
	objManager->pushGameObject(std::move(gameObject));

	AssetManager& assets = objManager->assetManager;

	// create terrain and place trees
	objManager->pushFuture(std::async(std::launch::async, [this, &device, &assets, id_terrain, chunkX, chunkY, chunkWorldSizeUnit]() {

		// voronoi map
		std::vector<std::vector<glm::vec2>> noiseMap(chunkSize, std::vector<glm::vec2>(chunkSize));
		
		for (int x = 0; x < chunkSize; x++) for (int y = 0; y < chunkSize; y++)
		{
			auto noiseHeight = VoronoiNoise::voronoi({ chunkX * (chunkSize - 1) + x,  chunkY * (chunkSize - 1) + y }, biomeScale);
			noiseMap[y][x] = glm::vec2{ std::min(1.f, noiseHeight[0].x / 1.18f), noiseHeight[0].y };
		}

		// create height map from noise function
		std::vector<std::vector<glm::vec2>> map = generateChunckHeight(chunkX * (chunkSize - 1), chunkY * (chunkSize - 1), chunkSize, chunkSize);
		std::vector<std::vector<float>> heightMap(chunkSize, std::vector<float>(chunkSize));
		for (int x = 0; x < chunkSize; x++) for (int y = 0; y < chunkSize; y++) 
		{
			// distance to the center
			float r = 1 - (glm::length(glm::vec2(x, y) - glm::vec2(chunkSize / 2, chunkSize / 2)) / (chunkSize/2));

			float x1 = map[y][x].x;
			x1 *= 3 * r * r - 2 * r * r * r; // smoothstep
			heightMap[x][y] = x1;
		}
	
		// create plane object
		std::shared_ptr<Model> plane = PrebuiltModel::createTerrain(device, assets, chunkWorldSizeUnit, chunkWorldSizeUnit, heightMap, 1.f);

		std::vector<std::vector<std::vector<float>>> heightMapTexture = std::vector<std::vector<std::vector<float>>>(chunkSize, std::vector<std::vector<float>>(chunkSize, std::vector<float>(2)));
		for (int x = 0; x < this->chunkSize; x++)
		for (int y = 0; y < this->chunkSize; y++)
		{
			heightMapTexture[y][x][0] = noiseMap[x][y].x;
			heightMapTexture[y][x][1] = noiseMap[x][y].y;
		}

		TextureBuilder builder(device);
		plane->setTexture(assets.textures().create((builder.fromVector(heightMapTexture))));


		return std::vector<futureObject>{futureObject{ plane, plane ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id_terrain, {}, false }};
		}));


	return { id_terrain };
}

std::vector<std::vector<glm::vec2>> finiteSizedTerrainGeneration::generateChunckHeight(float Xoffset, float Yoffset, uint16_t sizeX, uint16_t sizeY)
{
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

			//std::vector<glm::vec2> lookupVoronoi = VoronoiNoise::voronoi({ (int)((x + Xoffset)), (int)((y + Yoffset)) }, biomeScale, 1);
			
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

				noiseHeight += (perlinValue + .5f) * amplitude / (1 + m * weightedRegionValue({ {1, 0} }, &RegionVariables::gradientFactor));

				float amplitude1 = weightedRegionValue({ {1, 0} }, &RegionVariables::persistance);  // persistance : [0, 1]
				
				amplitude *= (amplitude1 / 5.f);  // average persistance
				
				frequency *= weightedRegionValue({ {1, 0} }, &RegionVariables::lacunarity);
			}

			noiseMap[x][y] = glm::vec2(globalHeightMultiplier * weightedRegionValue({{1, 0}}, &RegionVariables::heightMultiplier) * noiseHeight, slope);
		}
	}

	return noiseMap;

	//return std::vector<std::vector<glm::vec2>>{};
}

float finiteSizedTerrainGeneration::weightedRegionValue(const std::vector<glm::vec2>& lookupVoronoi, float RegionVariables::* member)
{
	float r = 0;
	float sum = 0;

	for (glm::vec2 val : lookupVoronoi) {
		const RegionVariables& reg = noiseVariable;
		r += reg.*member * val.x;
		sum += val.x;
	}

	return r / sum;
}