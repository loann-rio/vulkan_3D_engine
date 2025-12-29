#include "preBuild.h"

#include <random>
#include "../base/perlinNoise.h"
#include "../base/VoronoiNoise.h"

#include "../Textures/TextureBuilder.h"
#include "../Textures/TextureObject.h"

#include "Vertex/ObjVertexData.h"
#include "ModelAsset.h"
#include "ModelBuilder.h"

ModelManager::ModelID PrebuiltModel::createFullScreenQuad(Device& device, AssetManager& assets)
{
   
    std::vector<ObjVertex> vertices;
    vertices.push_back({ {-1.0f, -1.0f, 0.0f}, {1,1,1}, {0,0,1}, {0.0f, 0.0f} });  // bottom-left
    vertices.push_back({ { 1.0f, -1.0f, 0.0f}, {1,1,1}, {0,0,1}, {1.0f, 0.0f} });  // bottom-right
    vertices.push_back({ { 1.0f,  1.0f, 0.0f}, {1,1,1}, {0,0,1}, {1.0f, 1.0f} });  // top-right
    vertices.push_back({ {-1.0f,  1.0f, 0.0f}, {1,1,1}, {0,0,1}, {0.0f, 1.0f} });  // top-left

    auto vert = std::make_unique<ObjVertexData>(std::move(vertices));

    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 2, 3
    };

    TextureBuilder builder(device);
    auto text = assets.textures().create(
        builder.fromFile("textures/whiteTexture.jpg")
    );

    ModelBuilder modelBuilder(device, assets);
    auto model = assets.models().create(
        modelBuilder
        .fromVertexList(std::move(vert), indices)
        .withTexture(text)
    );
      

    return model;
}


ModelManager::ModelID PrebuiltModel::createPlane(Device& device, AssetManager& assets, float width, float depth, uint16_t widthDetail, uint16_t depthDetail, glm::vec3 color, float UVfactor)
{
    //Model::Builder modelBuilder{};
    std::vector<ObjVertex> vertices;
    // Step 1: Generate vertices
    for (unsigned int i = 0; i <= widthDetail; i++) {
        for (unsigned int j = 0; j <= depthDetail; j++) {

            glm::vec3 position = { i * width / widthDetail, 0.f, j * depth / depthDetail };
            glm::vec3 normal = { 0, 1, 0 };  // Upward-facing normal
            glm::vec2 uv = { (float)(i * UVfactor) / (float)widthDetail, (float)(j * UVfactor) / (float)depthDetail };

            vertices.push_back({ position, color, normal, uv });
        }
    }

    // Step 2: Generate indices (triangles)
    std::vector<uint32_t> indices;
    for (unsigned int i = 0; i < widthDetail; i++) {
        for (unsigned int j = 0; j < depthDetail; j++) {
            int topLeft = i * (depthDetail + 1) + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * (depthDetail + 1) + j;
            int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Step 3: Compute normals
    std::vector<glm::vec3> accumulatedNormals(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3& v0 = vertices[indices[i]].position;
        glm::vec3& v1 = vertices[indices[i + 1]].position;
        glm::vec3& v2 = vertices[indices[i + 2]].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        accumulatedNormals[indices[i]] += normal;
        accumulatedNormals[indices[i + 1]] += normal;
        accumulatedNormals[indices[i + 2]] += normal;
    }

    // Normalize accumulated normals
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::normalize(accumulatedNormals[i]);
    }

    TextureBuilder builder(device);
    auto text = assets.textures().create(
        builder.fromFile("textures/whiteTexture.jpg")
    );


    auto vert = std::make_unique<ObjVertexData>(std::move(vertices));

    ModelBuilder modelBuilder(device, assets);
    auto model = assets.models().create(
        modelBuilder
        .fromVertexList(std::move(vert), indices)
        .withTexture(text)
    );

    return model;
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
ModelManager::ModelID PrebuiltModel::createPlane(Device& device, AssetManager& assets, const unsigned int detail, const float sizePlane, glm::vec3 color, const std::string texturePath, float uvFactor)
{
    std::vector<ObjVertex> vertices;
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i < detail + 1; i++) {
        for (unsigned int j = 0; j < detail + 1; j++)
        {
            vertices.push_back({ {i * sizePlane / detail, 0.f, j * sizePlane / detail}, {1, 1, 1}, {0, -1, 0}, {(float)(i * uvFactor) / (float)detail , (float)(j * uvFactor) / (float)detail } });
        }
    }

    int row = 0;
    for (unsigned int i = 0; i < (detail + 1) * detail - 1; i++) {
        if ((i - row) % detail == 0 && i != 0) {
            i++;
            row++;
        }

        indices.push_back(i);
        indices.push_back(i + detail + 1);
        indices.push_back(i + 1);
        

        indices.push_back(i + 1);
        indices.push_back(i + detail + 1);
        indices.push_back(i + detail + 2);
        

        vertices[i].normal = -glm::normalize(glm::cross(vertices[i].position - vertices[i + 1].position, vertices[i].position - vertices[i + detail + 1].position));
    }

    TextureBuilder builder(device);
    auto text = assets.textures().create(
        builder.fromFile(texturePath.c_str())
    );

    auto vert = std::make_unique<ObjVertexData>(std::move(vertices));

    ModelBuilder modelBuilder(device, assets);
    auto model = assets.models().create(
        modelBuilder
        .fromVertexList(std::move(vert), indices)
        .withTexture(text)
    );

    return model;
}

std::shared_ptr<Model> PrebuiltModel::createIcoSphere(Device& device, AssetManager& assets, uint16_t detail)
{
    Model::Builder modelBuilder{};

    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    glm::vec3 normal = { 0, 1, 0 }; 
    glm::vec2 uv = { 0.1f, 0.1f };

    modelBuilder.vertices = {
        {{0.8506508f,           0.5257311f,         0.f},           color, normal, uv},
        {{0.000000101405476f,   0.8506507f,        -0.525731f},     color, normal, uv},
        {{0.000000101405476f,   0.8506506f,         0.525731f},     color, normal, uv},
        {{0.5257309f,          -0.00000006267203f, -0.85065067f},   color, normal, uv},
        {{0.52573115f,         -0.00000006267203f,  0.85065067f},   color, normal, uv},
        {{0.8506508f,          -0.5257311f,         0.f},           color, normal, uv},
        {{-0.52573115f,         0.00000006267203f, -0.85065067f},   color, normal, uv},
        {{-0.8506508f,          0.5257311f,         0.f},           color, normal, uv},
        {{-0.5257309f,          0.00000006267203f,  0.85065067f},   color, normal, uv},
        {{-0.000000101405476f, -0.8506506f,        -0.525731f},     color, normal, uv},
        {{-0.000000101405476f, -0.8506507f,         0.525731f},     color, normal, uv},
        {{-0.8506508f,         -0.5257311f,         0.f},           color, normal, uv}
    };

    modelBuilder.indices = {
             0,  1,  2,
             0,  3,  1,
             0,  2,  4,
             3,  0,  5,
             0,  4,  5,
             1,  3,  6,
             1,  7,  2,
             7,  1,  6,
             4,  2,  8,
             7,  8,  2,
             9,  3,  5,
             6,  3,  9,
             5,  4, 10,
             4,  8, 10,
             9,  5, 10,
             7,  6, 11,
             7, 11,  8,
            11,  6,  9,
             8, 11, 10,
            10, 11,  9
    };


    auto model = std::make_unique<Model>(device, assets, modelBuilder);

    TextureBuilder builder(device);
    model->setTexture(assets.textures().create((builder.fromFile("textures/floor.jpg"))));

    return model;
}

std::shared_ptr<Model> PrebuiltModel::createCube(Device& device, AssetManager& assets, uint16_t detail)
{
    return std::shared_ptr<Model>();
}

std::shared_ptr<Model> PrebuiltModel::createCube(Device& device, AssetManager& assets)
{
    Model::Builder modelBuilder{};
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    float h = 0.5f;

    // 8 shared vertices (corners)
    modelBuilder.vertices = {
        {{-h,-h,-h}, color, {-1,-1,-1}, {0,0}}, // 0
        {{ h,-h,-h}, color, { 1,-1,-1}, {1,0}}, // 1
        {{ h, h,-h}, color, { 1, 1,-1}, {1,1}}, // 2
        {{-h, h,-h}, color, {-1, 1,-1}, {0,1}}, // 3
        {{-h,-h, h}, color, {-1,-1, 1}, {0,0}}, // 4
        {{ h,-h, h}, color, { 1,-1, 1}, {1,0}}, // 5
        {{ h, h, h}, color, { 1, 1, 1}, {1,1}}, // 6
        {{-h, h, h}, color, {-1, 1, 1}, {0,1}}  // 7
    };

    // Indices for 12 triangles (2 per face)
    modelBuilder.indices = {
        // Front face
        4,5,6, 6,7,4,
        // Back face
        1,0,3, 3,2,1,
        // Left face
        0,4,7, 7,3,0,
        // Right face
        5,1,2, 2,6,5,
        // Top face
        3,7,6, 6,2,3,
        // Bottom face
        0,1,5, 5,4,0
    };

    // Recompute normals as average of faces for smooth shading:
    std::vector<glm::vec3> accum(modelBuilder.vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < modelBuilder.indices.size(); i += 3) {
        glm::vec3& p0 = modelBuilder.vertices[modelBuilder.indices[i]].position;
        glm::vec3& p1 = modelBuilder.vertices[modelBuilder.indices[i + 1]].position;
        glm::vec3& p2 = modelBuilder.vertices[modelBuilder.indices[i + 2]].position;
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        accum[modelBuilder.indices[i]] += normal;
        accum[modelBuilder.indices[i + 1]] += normal;
        accum[modelBuilder.indices[i + 2]] += normal;
    }
    for (size_t i = 0; i < modelBuilder.vertices.size(); i++) {
        modelBuilder.vertices[i].normal = glm::normalize(accum[i]);
    }

    auto model = std::make_unique<Model>(device, assets, modelBuilder);

    TextureBuilder builder(device);
    model->setTexture(assets.textures().create((builder.fromFile("textures/whiteTexture.jpg"))));

    return model;
}


std::shared_ptr<Model> PrebuiltModel::createTerrain(Device& device, AssetManager& assets,
    float width, float depth,
    uint16_t widthDetail, uint16_t depthDetail,
    float scale,
    uint16_t octaves ,
    float persistance, 
    float lacunarity, 
    float gradientFactor, 
    float heightMultiplier,
    float Xoffset, float Yoffset
    )
{

    PerlinNoise pn{ 3141592 };
    std::vector<std::vector<float>> noiseMap = pn.GenerateGradientTrick2DnoiseMap(widthDetail + 1, depthDetail + 1, scale, octaves, persistance, lacunarity, Xoffset, Yoffset, gradientFactor, heightMultiplier);

    Model::Builder modelBuilder{};

    float UVfactor = 1.f;;

    // Step 1: Generate vertices
    for (unsigned int i = 0; i <= widthDetail; i++) {
        for (unsigned int j = 0; j <= depthDetail; j++) {

            glm::vec3 position = { i * width / widthDetail, -noiseMap[j][i], j * depth / depthDetail};
            glm::vec3 color = { noiseMap[j][i], noiseMap[j][i], noiseMap[j][i] }; // White color
            glm::vec3 normal = { 0, -1, 0 };  // Upward-facing normal
            glm::vec2 uv = { (float)(i * UVfactor) / (float)widthDetail, (float)(j * UVfactor) / (float)depthDetail };

            modelBuilder.vertices.push_back({ position, color, normal, uv });
        }
    }

    // Step 2: Generate indices (triangles)
    for (unsigned int i = 0; i < widthDetail; i++) {
        for (unsigned int j = 0; j < depthDetail; j++) {
            int topLeft = i * (depthDetail + 1) + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * (depthDetail + 1) + j;
            int bottomRight = bottomLeft + 1;

            // First triangle
            modelBuilder.indices.push_back(topLeft);
            modelBuilder.indices.push_back(bottomLeft);
            modelBuilder.indices.push_back(topRight);

            // Second triangle
            modelBuilder.indices.push_back(topRight);
            modelBuilder.indices.push_back(bottomLeft);
            modelBuilder.indices.push_back(bottomRight);
        }
    }

    // Step 3: Compute normals
    std::vector<glm::vec3> accumulatedNormals(modelBuilder.vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < modelBuilder.indices.size(); i += 3) {
        glm::vec3& v0 = modelBuilder.vertices[modelBuilder.indices[i]].position;
        glm::vec3& v1 = modelBuilder.vertices[modelBuilder.indices[i + 1]].position;
        glm::vec3& v2 = modelBuilder.vertices[modelBuilder.indices[i + 2]].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        accumulatedNormals[modelBuilder.indices[i]] += normal;
        accumulatedNormals[modelBuilder.indices[i + 1]] += normal;
        accumulatedNormals[modelBuilder.indices[i + 2]] += normal;
    }


    // Normalize accumulated normals
    for (size_t i = 0; i < modelBuilder.vertices.size(); i++) {
        modelBuilder.vertices[i].normal = glm::normalize(accumulatedNormals[i]);

        if (modelBuilder.vertices[i].position.y < 2 && modelBuilder.vertices[i].normal.y < -0.7f) {
            modelBuilder.vertices[i].color = { 0.1, 0.7, 0.2 };
        }
        else
        {
            //modelBuilder.vertices[i].color = { 98, 70, 27 };
            modelBuilder.vertices[i].color = { 64.f, 61.f, 56.f };
            modelBuilder.vertices[i].color = modelBuilder.vertices[i].color / 255.f;
        }
    }

    auto model = std::make_unique<Model>(device, assets, modelBuilder);

    TextureBuilder builder(device);
    model->setTexture(assets.textures().create((builder.fromFile("textures/whiteTexture.jpg"))));

    return model;
}

std::shared_ptr<Model> PrebuiltModel::createTerrain(Device& device, AssetManager& assets, float width, float depth, std::vector<std::vector<float>> heightMap, float UVfactor)
{

    uint16_t widthDetail = heightMap.size() - 1;
    uint16_t depthDetail = heightMap[0].size() - 1;

    Model::Builder modelBuilder{};
    // Step 1: Generate vertices
    for (unsigned int i = 0; i <= widthDetail; i++) {
        for (unsigned int j = 0; j <= depthDetail; j++) {

            glm::vec3 position = { i * width / widthDetail, -heightMap[j][i], j * depth / depthDetail};
            glm::vec3 normal = { 0, 1, 0 };  // Upward-facing normal
            glm::vec2 uv = { (float)(i * UVfactor) / (float)widthDetail, (float)(j * UVfactor) / (float)depthDetail };

            modelBuilder.vertices.push_back({ position, {0, 0, 0}, normal, uv});
        }
    }

    // Step 2: Generate indices (triangles)
    for (unsigned int i = 0; i < widthDetail; i++) {
        for (unsigned int j = 0; j < depthDetail; j++) {
            int topLeft = i * (depthDetail + 1) + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * (depthDetail + 1) + j;
            int bottomRight = bottomLeft + 1;

            // First triangle
            modelBuilder.indices.push_back(topLeft);
            modelBuilder.indices.push_back(bottomLeft);
            modelBuilder.indices.push_back(topRight);

            // Second triangle
            modelBuilder.indices.push_back(topRight);
            modelBuilder.indices.push_back(bottomLeft);
            modelBuilder.indices.push_back(bottomRight);
        }
    }

    // Step 3: Compute normals
    std::vector<glm::vec3> accumulatedNormals(modelBuilder.vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < modelBuilder.indices.size(); i += 3) {
        glm::vec3& v0 = modelBuilder.vertices[modelBuilder.indices[i]].position;
        glm::vec3& v1 = modelBuilder.vertices[modelBuilder.indices[i + 1]].position;
        glm::vec3& v2 = modelBuilder.vertices[modelBuilder.indices[i + 2]].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        accumulatedNormals[modelBuilder.indices[i]] += normal;
        accumulatedNormals[modelBuilder.indices[i + 1]] += normal;
        accumulatedNormals[modelBuilder.indices[i + 2]] += normal;
    }

    // Normalize accumulated normals
    for (size_t i = 0; i < modelBuilder.vertices.size(); i++) {
        modelBuilder.vertices[i].normal = glm::normalize(accumulatedNormals[i]);

        float height = -1 * modelBuilder.vertices[i].position.y;
        float slope  = abs(modelBuilder.vertices[i].normal.y);

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

        modelBuilder.vertices[i].color = color;
    }

    auto model = std::make_unique<Model>(device, assets, modelBuilder);

    TextureBuilder builder(device);
    model->setTexture(assets.textures().create((builder.fromFile("textures/whiteTexture.jpg"))));

    return model;
}
