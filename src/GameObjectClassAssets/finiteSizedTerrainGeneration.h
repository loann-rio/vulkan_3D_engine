#pragma once

#include "../base/perlinNoise.h"
#include "../base/VoronoiNoise.h"

#include "../model/preBuild.h"
#include "../model/Model.h"

#include "../objects/objectManager.h"
#include "../base/Device.h"


class finiteSizedTerrainGeneration {

struct RegionVariables {
	float octaves;
	float persistance;
	float lacunarity;
	float gradientFactor;
	float heightMultiplier;
};

public:
	std::vector<GameObject::id_t> createChunk(Device& device, ObjectManager* objManager, int chunkX, int chunkY, float chunkWorldSizeUnit); 

private:

	std::vector<std::vector<glm::vec2>> generateChunckHeight(float Xoffset, float Yoffset, uint16_t sizeX, uint16_t sizeY);
	float weightedRegionValue(const std::vector<glm::vec2>& lookupVoronoi, float RegionVariables::* member);

	const unsigned int seed = 7653456789;
	float globalScale = 20.f;
	float biomeScale = 10;

	//float globalHeightMultiplier = 0.2 * 2 * (1 + 0.5 * sum(int(globalScale / 100) - 1));
	float globalHeightMultiplier = 0.1;

	PerlinNoise pn{ seed };

	const uint16_t chunkSize = 32; // number of vertices per side of a chunk

	RegionVariables noiseVariable{ 8, 0.45f, 2, 0, 3 };

	uint16_t maxOctaves = 8;

	int sum(int n) { return (n == 0) || (n == 1) ? 1 : n + sum(n - 1); }

};
