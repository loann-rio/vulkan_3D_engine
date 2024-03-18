#pragma once

#include "Model.h"

#include <iostream>
#include <glm/glm.hpp>
#include <memory>
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
static std::unique_ptr<Model> createPlane(Device& device, const int detail, const float sizePlane, glm::vec3 color) {
	Model::Builder modelBuilder{};

	for (unsigned int i = 0; i < detail + 1 ; i++) {
		for (unsigned int j = 0; j < detail + 1 ; j++) 
		{
			modelBuilder.vertices.push_back({ {i * sizePlane / detail, ((double)rand() / (RAND_MAX)), j * sizePlane / detail}, {((double)rand() / (RAND_MAX)), ((double)rand() / (RAND_MAX)), ((double)rand() / (RAND_MAX)) } });
		}
	}

    //for (unsigned int i = 1; i < detail - 1; i++) {
    //    for (unsigned int j = 1; j < detail - 1; j++) {
    //        glm::vec3 normal(0.0f, 0.0f, 0.0f); // Initialize normal to zero vector

    //        // Calculate normals of adjacent triangles and sum them up
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i - 1) + detail * (j - 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i)+detail * (j - 1)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i)+detail * (j - 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i + 1) + detail * (j - 1)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i + 1) + detail * (j - 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i + 1) + detail * (j)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i + 1) + detail * (j)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i + 1) + detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i + 1) + detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i)+detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i)+detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i - 1) + detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position));
    //        normal += glm::normalize(glm::cross(
    //            modelBuilder.vertices[(i - 1) + detail * (j + 1)].position - modelBuilder.vertices[i + detail * j].position,
    //            modelBuilder.vertices[(i - 1) + detail * (j)].position - modelBuilder.vertices[i + detail * j].position));

    //        // Normalize the sum to get the average normal
    //        modelBuilder.vertices[i + detail * j].normal = glm::normalize(normal);
    //    }
    //}

	int row = 0;
	for (int i = 0; i < (detail + 1 ) * detail - 1; i++) {
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


	std::cout << modelBuilder.vertices.size() << "\n";
	std::cout << modelBuilder.indices.size() << "\n";

	return std::make_unique<Model>(device, modelBuilder, "");
} 