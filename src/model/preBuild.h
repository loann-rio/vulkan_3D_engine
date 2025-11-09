#pragma once

#include "Model.h"

#include <iostream>
#include <glm/glm.hpp>
#include <memory>
#include <assert.h>
#include <vector>
#include <array>


class PrebuiltModel {
public:
    static std::shared_ptr<Model> createFullScreenQuad(Device& device);

    static std::shared_ptr<Model> createPlane(Device& device, float width, float depth, uint16_t widthDetail, uint16_t depthDetail, glm::vec3 color = { 1.0f, 1.0f, 1.0f }, float UVfactor = 1);
    static std::unique_ptr<Model> createPlane(Device& device, const unsigned int detail, const float sizePlane, glm::vec3 color, const std::string path = "textures/floor.jpg", float uvFactor = 1);

    static std::shared_ptr<Model> createIcoSphere(Device& device, uint16_t detail);

    static std::shared_ptr<Model> createCube(Device& device, uint16_t detail);
    static std::shared_ptr<Model> createCube(Device& device);

	static std::shared_ptr<Model> createCylinder(Device& device, float radiusTop, float radiusBottom, float height, uint16_t radialSegments, uint16_t heightSegments, bool openEnded = false, glm::vec3 color = { 1.0f, 1.0f, 1.0f }, float UVfactor = 1);
	static std::shared_ptr<Model> createCone(Device& device, float radius, float height, uint16_t radialSegments, uint16_t heightSegments, bool openEnded = false, glm::vec3 color = { 1.0f, 1.0f, 1.0f }, float UVfactor = 1);

    static std::shared_ptr<Model> createTerrain(Device& device, float width, float depth, uint16_t widthDetail, uint16_t depthDetail, float scale = 200.f, uint16_t octaves = 6, float persistance = 0.55f, float lacunarity = 2, float gradientFactor = 4400, float heightMultiplier = 1, float Xoffset = 0, float Yoffset = 0);
    static std::shared_ptr<Model> createTerrain(Device& device, float width, float depth, std::vector<std::vector<float>> heightMap, float UVfactor = 1);
};