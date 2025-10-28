#include "TerrainGenerator.h"

void TerrainGenerator::loop(Device& device, ObjectManager* objManager, GameObject* object)
{
	//int posX = object->transform.translation.x / chunkWorldSide;
	//int posY = object->transform.translation.y / chunkWorldSide;

	int posX = objManager->get("mainCamera")->transform.translation.x / chunkWorldSide;
	int posY = objManager->get("mainCamera")->transform.translation.z / chunkWorldSide;

	
	for (int i = posX - int(sizeWorldInChunck / 2); i <= posX + int(sizeWorldInChunck / 2); i++)
	for (int j = posY - int(sizeWorldInChunck / 2); j <= posY + int(sizeWorldInChunck / 2); j++)
	{
		int hash = i * 5463 + j * 9875;

		if (loadedChunk.count(hash) == 0) {
			
			std::cout << "chunk not found, creating chunk \n";
			
			
			// push terrain
			auto gameObject = GameObjectFactory::createGameObject<GameObjectModel>(device);
			gameObject->transform.translation = { i * chunkWorldSide, 4, j * chunkWorldSide };
			gameObject->setModelSubType(ModelSubType::TERRAIN);
			gameObject->setParent(objManager->get("terrain G"));
			GameObject::id_t id_terrain = gameObject->getId();

			objManager->pushGameObject(std::move(gameObject));

			// push Tree
			auto treesObject = GameObjectFactory::createGameObject<GameObjectModel>(device);
			treesObject->transform.translation = { j * chunkWorldSide, 0, i * chunkWorldSide };
			//treesObject->setName("trees");
			GameObject::id_t id_tree = treesObject->getId();

			objManager->pushGameObject(std::move(treesObject));

			// create terrain and place trees
			objManager->pushFuture(std::async(std::launch::async, [this, id_terrain, id_tree, i, j]() {

				// create height map from noise function
				std::vector<std::vector<glm::vec2>> heightMap = generateChunck(i * (this->chunkSize - 1), j * (this->chunkSize - 1));
				
				// separate height and slope
				std::vector<std::vector<float>> map = std::vector<std::vector<float>>(this->chunkSize, std::vector<float>(this->chunkSize));
				for (int x = 0; x < this->chunkSize; x++) for (int y = 0; y < this->chunkSize; y++) map[y][x] = heightMap[x][y].x;

				// create color map


				// create plane object
				std::shared_ptr<Model> plane = PrebuiltModel::createTerrain(this->device, 4, 4, map);

				std::unique_ptr<Texture> text = Texture::create(this->device, heightMap);
				//std::unique_ptr<Texture> text = Texture::create(this->device, "textures\\floor.jpg");
				if (text != nullptr) {
					plane->setTexture(std::move(text));
					std::cout << "texture loaded \n";
				}
				else std::cout << "texture not loaded \n";
				

				// place tree
				//std::vector<Model::Instance> treeList = this->placeTrees(heightMap, i, j);

				// load tree model
				//std::shared_ptr<Model> trees = Model::createModelFromFile(this->device, "model\\coloredTree1.obj", "textures\\whiteTexture.jpg");

				return std::vector<futureObject>{futureObject{ plane, plane ? ModelType::OBJ_MODEL : ModelType::UNDEFINED_MODEL, id_terrain, {}, false }};// , futureObject{ trees, trees ? OBJ_MODEL : UNDEFINED_MODEL, id_tree, treeList }};
				}));

			loadedChunk[hash] = { id_terrain, id_tree };

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
		if (validHashes.find(it->first) == validHashes.end()) {
			for (auto id : it->second) {
				objManager->removeGameObject(id);
				std::cout << "unload chunck \n";
			}

			it = loadedChunk.erase(it);
		}
		else {
			dynamic_cast<GameObjectModel*>(objManager->get(it->second.at(1)))->show = false;
			++it;
		}
	}

	int hash = posX * 5463 + posY * 9875;
	dynamic_cast<GameObjectModel*>(objManager->get(loadedChunk.at(hash).at(1)))->show = true; 
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
				
				amplitude *= weightedRegionValue(lookupVoronoi, &RegionVariables::persistance);  // persistance : [0, 1]
				frequency *= weightedRegionValue(lookupVoronoi, &RegionVariables::lacunarity);
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
