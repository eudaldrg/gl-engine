#include "gl_engine/viewer/camera.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gl_engine::viewer {

namespace {

constexpr glm::vec3 kUp{0.0F, 1.0F, 0.0F};
// Just short of 90 degrees, so the view never lines up with the up vector.
constexpr float kMaxPitch = glm::half_pi<float>() - 0.01F;
constexpr float kMinDistance = 0.05F;
constexpr float kZoomFactor = 0.9F;  // distance multiplier per zoom step

void Turn(float& yaw, float& pitch, float delta_yaw, float delta_pitch) {
    yaw += delta_yaw;
    pitch = std::clamp(pitch + delta_pitch, -kMaxPitch, kMaxPitch);
}

// Horizontal, so moving right never changes height.
glm::vec3 Right(float yaw, float pitch) {
    return glm::normalize(glm::cross(Forward(yaw, pitch), kUp));
}

}  // namespace

glm::vec3 Forward(float yaw, float pitch) {
    return {std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw)};
}

glm::vec3 OrbitCamera::Position() const {
    return target - distance * Forward(yaw, pitch);
}

glm::mat4 OrbitCamera::ViewMatrix() const {
    return glm::lookAt(Position(), target, kUp);
}

void OrbitCamera::Rotate(float delta_yaw, float delta_pitch) {
    Turn(yaw, pitch, delta_yaw, delta_pitch);
}

void OrbitCamera::Pan(glm::vec2 delta) {
    const glm::vec3 right = Right(yaw, pitch);
    const glm::vec3 view_up = glm::cross(right, Forward(yaw, pitch));
    target += distance * (delta.x * right + delta.y * view_up);
}

void OrbitCamera::Zoom(float steps) {
    distance = std::max(kMinDistance, distance * std::pow(kZoomFactor, steps));
}

glm::mat4 FlyCamera::ViewMatrix() const {
    return glm::lookAt(position, position + Forward(yaw, pitch), kUp);
}

void FlyCamera::Look(float delta_yaw, float delta_pitch) {
    Turn(yaw, pitch, delta_yaw, delta_pitch);
}

void FlyCamera::Move(glm::vec3 direction, float distance) {
    if (glm::length(direction) == 0.0F) {
        return;
    }
    const glm::vec3 world =
        direction.x * Right(yaw, pitch) + direction.y * kUp + direction.z * Forward(yaw, pitch);
    position += distance * glm::normalize(world);
}

}  // namespace gl_engine::viewer
