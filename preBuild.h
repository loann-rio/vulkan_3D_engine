#pragma once

#include "Model.h"

#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

//static std::unique_ptr<Model> createPlane(Device& device, int sizePlane, glm::vec3 color) {
//	Model::Builder modelBuilder{};
//
//	for (unsigned int i = 0; i < sizePlane; i++) {
//		for (unsigned int j = 0; j < sizePlane; j++) 
//		{
//			modelBuilder.vertices.push_back({ {(i - sizePlane / 2)/sizePlane, (j - sizePlane / 2)/sizePlane, 0.f}, color });
//		}
//	}
//
//	int ind = 0;
//	int row = 0;
//	for (unsigned int i = 0; i < sizePlane*(sizePlane-1); i++) {
//		modelBuilder.indices.push_back((row * sizePlane) + ind);
//		modelBuilder.indices.push_back((row * sizePlane) + ind + sizePlane);
//		modelBuilder.indices.push_back((row * sizePlane) + ind + sizePlane + 1);
//
//		modelBuilder.indices.push_back((row * sizePlane) + ind);
//		modelBuilder.indices.push_back((row * sizePlane) + ind + 1);
//		modelBuilder.indices.push_back((row * sizePlane) + ind + sizePlane + 1);
//
//		ind++;
//		if (ind == sizePlane) {
//			row++;
//			ind = 0;
//		}
//	}
//
//	std::cout << modelBuilder.vertices.size() << "\n";
//	std::cout << modelBuilder.indices.size() << "\n";
//
//	return std::make_unique<Model>(device, modelBuilder);
//}