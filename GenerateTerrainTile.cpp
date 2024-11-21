#include "GenerateTerrainTile.h"

void GenerateTerrainTile::generateHeightMap(Device& device)
{

    std::vector<std::vector<float>> noiseMap = pn.Generate2DnoiseMap(detailX + 1, detailY + 1, scale, octave, persistance, lacunarity, offsetX, offsetY);

    unsigned char* buffer = new unsigned char[(detailX + 1) * (detailY + 1) * 4];
    unsigned char* traverseBuffer = buffer;


    for (int32_t x = 0; x < detailX + 1; ++x) {
        for (int32_t y = 0; y < detailY + 1; ++y) {

            for (int32_t j = 0; j < 3; ++j) {
                
                traverseBuffer[j] = noiseMap[y][x] * 255;
            }

            traverseBuffer[3] = heightTransform(noiseMap[y][x]) * 255;
            traverseBuffer += 4;
        }
    }

    noiseTexture = std::make_shared<Texture>(device, buffer, detailX + 1, detailY + 1);

    delete[] buffer;
}

std::shared_ptr<Model> GenerateTerrainTile::generateMesh(Device& device)
{
    Model::Builder modelBuilder{};

    for (uint16_t x = 0; x < detailX + 1; x++)
    {
        for (uint16_t y = 0; y < detailY + 1; y++)
        {
            modelBuilder.vertices.push_back({ {x * sizePlaneX / detailX, 0, y * sizePlaneY / detailY}, {0, 0, 0} , {0.0, -1.0, 0.0}, {(float)(x) / (float)detailX , (float)(y) / (float)detailY} });
        }
    }
    
    int row = 0;
    for (int i = 0; i < (detailX + 1) * detailX - 1; i++) {
        if ((i - row) % detailX == 0 && i != 0) {
            i++;
            row++;
        }

        modelBuilder.indices.push_back(i);
        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detailX + 1);

        modelBuilder.indices.push_back(i + 1);
        modelBuilder.indices.push_back(i + detailX + 2);
        modelBuilder.indices.push_back(i + detailX + 1);

        modelBuilder.vertices[i].normal = -glm::normalize(glm::cross(modelBuilder.vertices[i].position - modelBuilder.vertices[i + 1].position, modelBuilder.vertices[i].position - modelBuilder.vertices[i + detailX + 1].position));

    }

    return std::make_unique<Model>(device, modelBuilder, "textures/floor.jpg");

}
