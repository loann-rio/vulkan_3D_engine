#pragma once

#include "Model.h"
#include "PerlinNoise.h"

#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct terrainType {
    float height;
    glm::vec3 color{};
};



static std::unique_ptr<Model> createPlane1(Device& device, const int detail, const float sizePlane, glm::vec3 color = { 1.0f, 1.0f, 1.0f })
{

    Model::Builder modelBuilder{};

    for (unsigned int i = 0; i < detail + 1; i++)
    {
        for (unsigned int j = 0; j < detail + 1; j++)
        {
            modelBuilder.vertices.push_back({ {i * sizePlane / detail, 0.0, j * sizePlane / detail}, color , { 0.0, -1.0, 0.0 } });
        }
    }

    int row = 0;
    for (int i = 0; i < (detail + 1) * detail - 1; i++) {
        if ((i - row) % detail == 0 && i != 0) {
            i++;
            row++;
        }

        modelBuilder.indices.push_back(i);
        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detail + 1);

        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detail + 2);
        modelBuilder.indices.push_back(i + detail + 1);

    }
}



/// <summary>
/// create a terrain made of triangles
/// </summary>
/// <param name="device"></param>
/// <param name="detail"> length of the plane in term of triangles </param>
/// <param name="sizePlane"> length of the plane in term of pixels </param>
/// <param name="color"></param>
/// <returns> pointer to a new model </returns>
/// 
static std::unique_ptr<Model> createTerrain(Device& device, const int detail, const float sizePlane, glm::vec3 color) {


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



    float heightMultiplier = 3.f;


    PerlinNoise pn{ 3141592 };
    std::vector<std::vector<float>> noiseMap = pn.Generate2DnoiseMap(detail + 1, detail + 1, 20.f, 6, 0.55f, 2, 0, 10);

    Model::Builder modelBuilder{};

    for (unsigned int i = 0; i < detail + 1; i++)
    {
        for (unsigned int j = 0; j < detail + 1; j++)
        {
            float height = noiseMap[i][j];


            glm::vec3 color{};
            for (const terrainType tt : regions)
            {
                if (height <= tt.height) {
                    color = tt.color;
                    break;
                }
            }

            height = pow(height, 3);

            modelBuilder.vertices.push_back({ {i * sizePlane / detail, height * heightMultiplier, j * sizePlane / detail}, color , { 0.0, -1.0, 0.0 } });
        }
    }


   

    int row = 0;
    for (int i = 0; i < (detail + 1) * detail - 1; i++) {
        if ((i - row) % detail == 0 && i != 0) {
            i++;
            row++;
        }

        modelBuilder.indices.push_back(i);
        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detail + 1);

        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detail + 2);
        modelBuilder.indices.push_back(i + detail + 1);

        if (modelBuilder.vertices[i].position.y > 0.23)
            modelBuilder.vertices[i].normal = -glm::normalize(glm::cross(modelBuilder.vertices[i].position - modelBuilder.vertices[i + 1].position, modelBuilder.vertices[i].position - modelBuilder.vertices[i + detail + 1].position));

    }


    std::cout << modelBuilder.vertices.size() << "\n";
    std::cout << modelBuilder.indices.size() << "\n";

    return std::make_unique<Model>(device, modelBuilder, "");
}