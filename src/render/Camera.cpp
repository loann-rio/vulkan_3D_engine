#include "Camera.h"

#include <cassert>
#include <limits>

void Camera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
    projectionMatrix = glm::mat4{ 1.0f };
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (bottom - top);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    projectionMatrix[3][2] = -near / (far - near);
}

void Camera::setPerspectiveProjection(float fov, float aspect_ratio, float near, float far) {
    assert(glm::abs(aspect_ratio - std::numeric_limits<float>::epsilon()) > 0.0f);
    _fov = fov;
    _near = near;
    _far = far;
    aspectRatio = aspect_ratio;

    const float tanHalfFovy = tan(fov / 2.f);
    projectionMatrix = glm::mat4{ 0.0f };
    projectionMatrix[0][0] = 1.f / (aspect_ratio * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}

void Camera::setPerspectiveProjection(float aspect_ratio)
{
    setPerspectiveProjection(_fov, aspect_ratio, _near, _far);
    aspectRatio = aspect_ratio;
}

void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    const glm::vec3 w{ glm::normalize(direction) };
    const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
    const glm::vec3 v{ glm::cross(w, u) };

    viewMatrix = glm::mat4{ 1.f };
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);

    inverseViewMatrix = glm::mat4{ 1.f };
    inverseViewMatrix[0][0] = u.x;
    inverseViewMatrix[0][1] = u.y;
    inverseViewMatrix[0][2] = u.z;
    inverseViewMatrix[1][0] = v.x;
    inverseViewMatrix[1][1] = v.y;
    inverseViewMatrix[1][2] = v.z;
    inverseViewMatrix[2][0] = w.x;
    inverseViewMatrix[2][1] = w.y;
    inverseViewMatrix[2][2] = w.z;
    inverseViewMatrix[3][0] = position.x;
    inverseViewMatrix[3][1] = position.y;
    inverseViewMatrix[3][2] = position.z;
}

void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    setViewDirection(position, target - position, up);
}

void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
    const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
    viewMatrix = glm::mat4{ 1.f };
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);

    inverseViewMatrix = glm::mat4{ 1.f };
    inverseViewMatrix[0][0] = u.x;
    inverseViewMatrix[0][1] = u.y;
    inverseViewMatrix[0][2] = u.z;
    inverseViewMatrix[1][0] = v.x;
    inverseViewMatrix[1][1] = v.y;
    inverseViewMatrix[1][2] = v.z;
    inverseViewMatrix[2][0] = w.x;
    inverseViewMatrix[2][1] = w.y;
    inverseViewMatrix[2][2] = w.z;
    inverseViewMatrix[3][0] = position.x;
    inverseViewMatrix[3][1] = position.y;
    inverseViewMatrix[3][2] = position.z;
}

void Camera::updateFrustrumPlanes()
{
    glm::mat4 combo = projectionMatrix * viewMatrix;

    // Left
    frustrum[0].equation = glm::vec4(
        combo[0][3] + combo[0][0],
        combo[1][3] + combo[1][0],
        combo[2][3] + combo[2][0],
        combo[3][3] + combo[3][0]);

    // Right
    frustrum[1].equation = glm::vec4(
        combo[0][3] - combo[0][0],
        combo[1][3] - combo[1][0],
        combo[2][3] - combo[2][0],
        combo[3][3] - combo[3][0]);

    // Bottom
    frustrum[2].equation = glm::vec4(
        combo[0][3] + combo[0][1],
        combo[1][3] + combo[1][1],
        combo[2][3] + combo[2][1],
        combo[3][3] + combo[3][1]);

    // Top
    frustrum[3].equation = glm::vec4(
        combo[0][3] - combo[0][1],
        combo[1][3] - combo[1][1],
        combo[2][3] - combo[2][1],
        combo[3][3] - combo[3][1]);

    // Near
    frustrum[4].equation = glm::vec4(
        combo[0][3] + combo[0][2],
        combo[1][3] + combo[1][2],
        combo[2][3] + combo[2][2],
        combo[3][3] + combo[3][2]);

    // Far
    frustrum[5].equation = glm::vec4(
        combo[0][3] - combo[0][2],
        combo[1][3] - combo[1][2],
        combo[2][3] - combo[2][2],
        combo[3][3] - combo[3][2]);

    // Normalize all planes
    for (auto& p : frustrum) p.normalize();
}

const bool Camera::isAABBinFrustrum(const BoundingBox& aabb, const std::array<FrustumPlane, 6>& planes)
{
    glm::vec3 min = aabb.min;
    glm::vec3 max = aabb.max;

    // For each plane, check if any corner of the box is in front of it.
    for (const auto& plane : planes) {
        // Compute the positive vertex (the corner most in the direction of the plane normal)
        glm::vec3 positiveVertex = min;

        if (plane.equation.x >= 0) positiveVertex.x = max.x;
        if (plane.equation.y >= 0) positiveVertex.y = max.y;
        if (plane.equation.z >= 0) positiveVertex.z = max.z;

        // If positive vertex is still behind the plane, the whole box is outside
        if (plane.distanceToPoint(positiveVertex) < 0)
            return false;
    }

    return true; // Box is at least partially inside
}