// Orbit and fly cameras. Both use the same angles: yaw around +y, pitch up from the horizon, and
// yaw = pitch = 0 looks down -z.
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace gl_engine::viewer {

// Unit vector the camera looks along for the given angles, in radians.
glm::vec3 Forward(float yaw, float pitch);

// Circles a target point at a distance.
class OrbitCamera {
  public:
    glm::vec3 target{0.0F};
    float distance = 5.0F;
    float yaw = 0.0F;
    float pitch = 0.0F;

    glm::vec3 Position() const;
    glm::mat4 ViewMatrix() const;

    // Angles in radians; pitch stops short of straight up or down.
    void Rotate(float delta_yaw, float delta_pitch);
    // Moves the target in the view plane, by a fraction of the distance per unit of `delta`.
    void Pan(glm::vec2 delta);
    // Positive steps move closer. The distance scales, so zooming feels the same at any range.
    void Zoom(float steps);
};

// Free camera moved with keys and turned with the mouse.
class FlyCamera {
  public:
    glm::vec3 position{0.0F};
    float yaw = 0.0F;
    float pitch = 0.0F;

    glm::mat4 ViewMatrix() const;

    void Look(float delta_yaw, float delta_pitch);
    // `direction` is in camera space: x right, y world up, z forward. Moves `distance` along it.
    void Move(glm::vec3 direction, float distance);
};

}  // namespace gl_engine::viewer
