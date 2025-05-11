#include "preBuild.h"

std::shared_ptr<Model> PrebuiltModel::createPlane(Device& device, float width, float depth, uint16_t widthDetail, uint16_t depthDetail, float UVfactor)
{
    Model::Builder modelBuilder{};

    // Step 1: Generate vertices
    for (unsigned int i = 0; i <= widthDetail; i++) {
        for (unsigned int j = 0; j <= depthDetail; j++) {

            glm::vec3 position = { i * width / widthDetail, 0.f, j * depth / depthDetail };
            glm::vec3 color = { 1.0f, 1.0f, 1.0f }; // White color
            glm::vec3 normal = { 0, 1, 0 };  // Upward-facing normal
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
    }

    return std::make_shared<Model>(device, modelBuilder, "");
}

std::shared_ptr<Model> PrebuiltModel::createIcoSphere(Device& device, float radius, uint16_t detail)
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


    return std::make_shared<Model>(device, modelBuilder, "textures/floor.jpg"); 
}
