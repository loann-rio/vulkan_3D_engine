#pragma once

#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "../model/BoundingBox.h"

struct FrustumPlane {
	glm::vec4 equation; // (a, b, c, d) where ax + by + cz + d = 0

	void normalize() {
		float length = glm::length(glm::vec3(equation));
		equation /= length;
	}

	float distanceToPoint(const glm::vec3& point) const {
		return glm::dot(glm::vec3(equation), point) + equation.w;
	}
};

class Camera
{
public:
	Camera(float aspectRatio) : aspectRatio { aspectRatio } {}
	Camera() {} 

	void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
	void setPerspectiveProjection(float fov, float aspect_ratio, float near, float far);
	void setPerspectiveProjection(float aspect_ratio);

	void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
	void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
	void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
	
	void updateFrustrumPlanes();

	const glm::mat4& getProjection() const { return projectionMatrix;  }
	const glm::mat4& getView() const { return viewMatrix; }
	const glm::mat4& getInverseView() const { return inverseViewMatrix; }
	const glm::vec3 getPosition() const { return glm::vec3(inverseViewMatrix[3]); }
	const std::array<FrustumPlane, 6>& getFrustum() const { return frustrum; }
	
	float aspectRatio = 1.0f;
	float _fov, _near, _far;

	static const bool isAABBinFrustrum(const BoundingBox& aabb, const std::array<FrustumPlane, 6>& planes);

private:
	std::array<FrustumPlane, 6> frustrum;

	glm::mat4 projectionMatrix{ 1.f };
	glm::mat4 viewMatrix{ 1.f };
	glm::mat4 inverseViewMatrix{ 1.f };
};

