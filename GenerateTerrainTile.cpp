#include "GenerateTerrainTile.h"

void GenerateTerrainTile::generateHeightMap(Device& device)
{

    std::vector<std::vector<float>> noiseMap = pn.Generate2DnoiseMap(detailX + 1, detailZ + 1, scale, octave, persistance, lacunarity, offsetX, offsetZ);
                 
    unsigned char* buffer = new unsigned char[(detailX + 1) * (detailZ + 1) * 4];
    unsigned char* traverseBuffer = buffer;


    for (int32_t x = 0; x < detailX + 1; ++x) {
        for (int32_t y = 0; y < detailZ + 1; ++y) {

            glm::vec3 color = getHeigthColor(noiseMap[y][x]);

            traverseBuffer[0] = color.x * 255;
            traverseBuffer[1] = color.y * 255;
            traverseBuffer[2] = color.z * 255;            
            traverseBuffer[3] = heightTransform(noiseMap[y][x]) * 255;
            traverseBuffer += 4;
        }
    }

    noiseTexture = std::make_shared<Texture>(device, buffer, detailX + 1, detailZ + 1);

    delete[] buffer;
}

glm::vec3 GenerateTerrainTile::getHeigthColor(float height)
{
    for (terrainType tt : regions) {
        if (tt.height > height)
            return tt.color;
    }
}

std::unique_ptr<Model> GenerateTerrainTile::generateMesh(Device& device)
{
    Model::Builder modelBuilder{};

    for (unsigned int i = 0; i <= detailX ; i++) {
        for (unsigned int j = 0; j <= detailZ; j++)
        {
            modelBuilder.vertices.push_back({ {i * sizePlaneX / detailX, 0.f, j * sizePlaneZ / detailZ}, {0, 0, 0}, {0, -1, 0}, {(float)(i) / (float)(detailX + 1), (float)(j) / (float)(detailZ + 1)} });
        }
    }

    int row = 0;
    for (unsigned int i = 0; i < (detailX + 1) * detailX - 1; i++) {
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
   