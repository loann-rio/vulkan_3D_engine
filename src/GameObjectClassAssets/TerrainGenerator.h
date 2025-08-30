#pragma once

#include "../base/perlinNoise.h"
#include "../base/VoronoiNoise.h"
#include "../base/Device.h"
#include "../model/preBuild.h"
#include "../model/Model.h"

#include "../objects/objectManager.h"

#include <map>
#include <unordered_set>
#include <array>


enum RegionType {
	Montain = 0,
	Sea = 1,
	Planes = 2,
};

struct RegionVariables {
	RegionType regionType;
	float octaves;
	float persistance;
	float lacunarity;
	float gradientFactor;
	float heightMultiplier;
};


class TerrainGenerator : public GameObjectBehavior 
{
public:
	void setup(Device& device, ObjectManager* objManager, GameObject* object) override {}
	void loop(Device& device, ObjectManager* objManager, GameObject* object) override;

	TerrainGenerator(Device& device);
	std::vector<std::vector<glm::vec2>> generateChunck(float Xoffset, float Yoffset);
	std::vector<Model::Instance> placeTrees(std::vector<std::vector<glm::vec2>> heightMap, float Xoffset, float Yoffset) const;

	const uint32_t sizeWorldInChunck = 4;
	const uint16_t chunkSize = 158;
	const int treeProbability = 20; // between 0 - 100

private:
	const unsigned int seed = 314151;
	float globalScale = 200.f;

	Device& device;
	
	float globalHeightMultiplier = 0.2 * 2 * (1 + 0.5 * sum(int(globalScale / 100) - 1));
	

	float biomeScale = 300;
	const uint16_t numberOfBiome = 5;

	uint16_t maxOctaves = 8;

	const float chunkWorldSide = 4.f;

	std::map<int, std::array<GameObject::id_t, 2>> loadedChunk{};

	RegionVariables montains{ Montain, 8, 0.45f, 2, 4400, 6 };	
	RegionVariables planes{ Planes, 3, 0.35f, 2, 0, 1.5 };
	//RegionVariables planes{ Planes, 6, 0.35f, 2, 4400, 1.5 };
	
	std::vector<RegionVariables> regions = { montains, planes };

	PerlinNoise pn{ seed };

	std::vector<std::vector<glm::vec3>> generatecolorMap(std::vector<std::vector<glm::vec2>> heightMap);

	float weightedRegionValue(const std::vector<glm::vec2>& lookupVoronoi, float RegionVariables::* member);
	int sum(int n) { return (n == 0) || (n == 1) ? 1 : n + sum(n - 1); }



	// objects:

	// tree model
	//std::shared_ptr<Model> treeModel = Model::createModelFromFile(device, "C:\\Users\\riolo\\Desktop\\vulkan_3D_engine\\model\\coloredTree1.obj", "textures\\whiteTexture.jpg");

};

