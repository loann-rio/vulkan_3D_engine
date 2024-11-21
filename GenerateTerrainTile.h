#pragma once


#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <assert.h>
#include <functional>
#include <vector>

#include "Texture.h"
#include "Model.h"

#include "PerlinNoise.h" 


class GenerateTerrainTile
{

private:
    //std::vector<terrainType> heightRegions;

    const uint32_t seed = 33456789;

    std::vector<std::vector<float>> HeightMap;

    std::function<float(float)> heightTransform = [](float x) { return 1 - pow(x, 3); };

    int x = 0;
    int y = 0;
    int z = 0;

    int detailX = 100;
    int detailY = 100;
    int octave = 6;
    float scale = 20.f;
    float persistance = 0.55f;
    float lacunarity = 2;
    float offsetX = 0;
    float offsetY = 0;
    float heightMultiplier = 3.f;


    const int sizePlaneX = 10;
    const int sizePlaneY = 10;

    PerlinNoise pn{ seed };
public:

    void generateHeightMap(Device& device);
    void generateColorMap(Device& device);

    std::shared_ptr<Model> generateMesh(Device& device);

    std::shared_ptr<Texture> noiseTexture;

    GenerateTerrainTile() {};

};

