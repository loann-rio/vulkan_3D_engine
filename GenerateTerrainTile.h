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

    struct terrainType {
        float height;
        glm::vec3 color{};
    };

    std::vector<terrainType> regions{
           {.3f  , {.137f, .192f, .596f }},
           {.4f  , {.145f, .271f, .659f }},
           {.5f , {.878f, .823f, .4f   }},
           {.6f , {.145f, .659f, .188f }},
           {.7f  , {.176f, .557f, .208f }},
           {.8f  , {.482f, .376f, .247f }},
           {.9f , {.443f, .322f, .212f }},
           {1.f  , {1.f  , 1.f  , 1.f   }},
    };

    const uint32_t seed = 3346789;

    std::vector<std::vector<float>> HeightMap;

    std::function<float(float)> heightTransform = [](float x) { return 1 - pow(x, 3); };

    int x = 0;
    int y = 0;
    int z = 0;

    
    int octave = 1;
    float persistance = 0.55f;
    float lacunarity = 2;
    
    float heightMultiplier = 3.f;


    const float sizePlaneX = 5;
    const float sizePlaneY = 5;

    int posX = 0;
    int posY = 0;

    PerlinNoise pn{ seed };
public:
    float offsetX = 0;
    float offsetY = 0;

    int detailX = 10;
    int detailY = 10;

    float scale = 20.f;


    void generateHeightMap(Device& device);
    glm::vec3 getHeigthColor(float height);

    std::unique_ptr<Model> generateMesh(Device& device);

    std::shared_ptr<Texture> noiseTexture;

    GenerateTerrainTile() {};

};

