// Interactive state between the host and the renderer: cameras driven by input events, display
// toggles, frame stats and the panel that shows them.
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <optional>

#include "gl_engine/renderer/renderer.h"
#include "gl_engine/viewer/camera.h"
#include "gl_engine/viewer/frame_stats.h"
#include "gl_engine/viewer/input.h"

namespace gl_engine::viewer {

enum class CameraMode : std::uint8_t { kOrbit, kFly };

class Viewer {
  public:
    // The size of the area the host renders into, in pixels.
    Viewer(int width, int height);

    // Mouse and keyboard events are ignored while the ImGui panel wants them.
    void HandleEvent(const InputEvent& event);

    // Once per frame with the time since the previous frame (std::chrono::steady_clock).
    void Update(std::chrono::steady_clock::duration frame_time);

    renderer::View BuildView() const;

    // Draws the panel; call between Gui::NewFrame and Gui::Render.
    void DrawPanel();

    float Aspect() const {
        return aspect_;
    }
    CameraMode Mode() const {
        return mode_;
    }
    const OrbitCamera& Orbit() const {
        return orbit_;
    }
    const FlyCamera& Fly() const {
        return fly_;
    }
    const FrameStats& Stats() const {
        return stats_;
    }

  private:
    static constexpr float kStartFlySpeed = 3.0F;
    static constexpr float kStartLightAzimuth = glm::radians(146.0F);
    static constexpr float kStartLightElevation = glm::radians(54.0F);

    void OnMouseMove(glm::vec2 position);
    void OnKey(const Key& key);
    void ToggleMode();

    float aspect_;
    CameraMode mode_ = CameraMode::kOrbit;
    OrbitCamera orbit_;
    FlyCamera fly_;
    float fly_speed_ = kStartFlySpeed;  // units per second

    std::optional<glm::vec2> last_mouse_;
    std::array<bool, 3> buttons_{};  // indexed by Button
    std::array<bool, 8> keys_{};     // indexed by KeyCode
    bool sprint_ = false;

    bool wireframe_ = false;
    bool show_grid_ = true;
    bool show_axes_ = true;
    float light_azimuth_ = kStartLightAzimuth;      // radians, where the light comes from
    float light_elevation_ = kStartLightElevation;  // radians above the horizon

    FrameStats stats_;
};

}  // namespace gl_engine::viewer
