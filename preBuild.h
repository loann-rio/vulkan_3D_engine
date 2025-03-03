#pragma once

#include "Model.h"

#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <assert.h>
#include <vector>




/// <summary>
/// create a terrain made of triangles
/// </summary>
/// <param name="device"></param>
/// <param name="detail"> length of the plane in term of triangles </param>
/// <param name="sizePlane"> length of the plane in term of pixels </param>
/// <param name="color"></param>
/// <returns> pointer to a new model </returns>
/// 
static std::unique_ptr<Model> createPlane(Device& device, const unsigned int detail, const float sizePlane, glm::vec3 color, const char* path = "textures/floor.jpg") {
    Model::Builder modelBuilder{};

    for (unsigned int i = 0; i < detail + 1; i++) {
        for (unsigned int j = 0; j < detail + 1; j++)
        {
            modelBuilder.vertices.push_back({ {i * sizePlane / detail, 0.f, j * sizePlane / detail}, {0, 0, 0}, {0, -1, 0}, {(float) (i * 8) / (float) detail , (float) (j * 8) / (float) detail } });
        }
    }

    int row = 0;
    for (unsigned int i = 0; i < (detail + 1) * detail - 1; i++) {
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

        modelBuilder.vertices[i].normal = -glm::normalize(glm::cross(modelBuilder.vertices[i].position - modelBuilder.vertices[i + 1].position,   modelBuilder.vertices[i].position - modelBuilder.vertices[i + detail + 1].position));
    }

    return std::make_unique<Model>(device, modelBuilder, path);
}


/*
Model::Builder modelBuilder{};
    modelBuilder.vertices = {
        {{-1.0f,  1.0f, 0.f}, {0, 0, 0}, {0, 0, 0}, { 0.0f, 1.0f }}, // Top-left
        {{-0.6f,  1.0f, 0.f}, {0, 0, 0}, {0, 0, 0}, {1.0f, 1.0f}}, // Top-right
        {{-1.0f,  0.6f, 0.f}, {0, 0, 0}, {0, 0, 0}, {0.0f, 0.0f}}, // Bottom-left
        {{-0.6f,  0.6f, 0.f}, {0, 0, 0}, {0, 0, 0}, {1.0f, 0.0f}}  // Bottom-right
    };

    modelBuilder.indices = {
        0, 1, 2, // First triangle
        2, 1, 3  // Second triangle
    };*/