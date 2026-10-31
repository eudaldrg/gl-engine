#include "gl_engine/viewer/viewer.h"

#include <imgui.h>

#include <cmath>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <utility>
#include <variant>

namespace gl_engine::viewer {

namespace {

constexpr float kFieldOfView = glm::radians(45.0F);
constexpr float kNear = 0.05F;
constexpr float kFar = 200.0F;

constexpr float kRotateSpeed = 0.005F;  // radians per pixel
constexpr float kPanSpeed = 0.0015F;    // fractions of the orbit distance per pixel
constexpr float kSprintFactor = 4.0F;
constexpr float kFlySpeedStep = 1.1F;  // per scroll step

// The start view frames the test scene from above and to the right.
constexpr glm::vec3 kStartTarget{0.0F, 0.3F, 0.0F};
constexpr float kStartDistance = 7.8F;
constexpr float kStartYaw = glm::radians(-21.0F);
constexpr float kStartPitch = glm::radians(-25.0F);

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

bool ImGuiWantsMouse() {
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiWantsKeyboard() {
    return ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}

}  // namespace

Viewer::Viewer(int width, int height)
    : aspect_{height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0F} {
    orbit_.target = kStartTarget;
    orbit_.distance = kStartDistance;
    orbit_.yaw = kStartYaw;
    orbit_.pitch = kStartPitch;
}

void Viewer::HandleEvent(const InputEvent& event) {
    std::visit(Overloaded{
                   [this](const MouseMove& move) { OnMouseMove(move.position); },
                   [this](const MouseButton& button) {
                       // Releases always go through, so a drag that ends over the panel ends.
                       if (!button.pressed || !ImGuiWantsMouse()) {
                           buttons_.at(std::to_underlying(button.button)) = button.pressed;
                       }
                   },
                   [this](const Scroll& scroll) {
                       if (ImGuiWantsMouse()) {
                           return;
                       }
                       if (mode_ == CameraMode::kOrbit) {
                           orbit_.Zoom(scroll.delta);
                       } else {
                           fly_speed_ *= std::pow(kFlySpeedStep, scroll.delta);
                       }
                   },
                   [this](const Key& key) { OnKey(key); },
                   [this](const Resize& resize) {
                       // A minimised window reports zero; keep the last usable aspect.
                       if (resize.width > 0 && resize.height > 0) {
                           aspect_ =
                               static_cast<float>(resize.width) / static_cast<float>(resize.height);
                       }
                   },
               },
               event);
}

void Viewer::OnMouseMove(glm::vec2 position) {
    const std::optional<glm::vec2> last = std::exchange(last_mouse_, position);
    if (!last) {
        return;
    }
    const glm::vec2 delta = position - *last;
    const bool left = buttons_.at(std::to_underlying(Button::kLeft));
    const bool pan = buttons_.at(std::to_underlying(Button::kRight)) ||
                     buttons_.at(std::to_underlying(Button::kMiddle));
    if (mode_ == CameraMode::kOrbit) {
        if (left) {
            orbit_.Rotate(delta.x * kRotateSpeed, -delta.y * kRotateSpeed);
        } else if (pan) {
            orbit_.Pan({-delta.x * kPanSpeed, delta.y * kPanSpeed});
        }
    } else if (left || pan) {
        fly_.Look(delta.x * kRotateSpeed, -delta.y * kRotateSpeed);
    }
}

void Viewer::OnKey(const Key& key) {
    sprint_ = key.mods.shift;
    if (key.pressed && ImGuiWantsKeyboard()) {
        return;
    }
    keys_.at(std::to_underlying(key.key)) = key.pressed;
    if (key.key == KeyCode::kTab && key.pressed) {
        ToggleMode();
    }
}

void Viewer::ToggleMode() {
    if (mode_ == CameraMode::kOrbit) {
        // Start flying from where the orbit camera is, looking the same way.
        fly_.position = orbit_.Position();
        fly_.yaw = orbit_.yaw;
        fly_.pitch = orbit_.pitch;
        mode_ = CameraMode::kFly;
    } else {
        mode_ = CameraMode::kOrbit;
    }
}

void Viewer::Update(std::chrono::steady_clock::duration frame_time) {
    stats_.Add(frame_time);
    if (mode_ != CameraMode::kFly) {
        return;
    }
    const auto held = [this](KeyCode key) {
        return keys_.at(std::to_underlying(key)) ? 1.0F : 0.0F;
    };
    const glm::vec3 direction{held(KeyCode::kD) - held(KeyCode::kA),
                              held(KeyCode::kE) - held(KeyCode::kQ),
                              held(KeyCode::kW) - held(KeyCode::kS)};
    const float seconds = std::chrono::duration<float>(frame_time).count();
    fly_.Move(direction, fly_speed_ * (sprint_ ? kSprintFactor : 1.0F) * seconds);
}

renderer::View Viewer::BuildView() const {
    return {
        .view = mode_ == CameraMode::kOrbit ? orbit_.ViewMatrix() : fly_.ViewMatrix(),
        .projection = glm::perspective(kFieldOfView, aspect_, kNear, kFar),
        // The light travels from where it comes from towards the scene.
        .light_direction = -Forward(light_azimuth_, light_elevation_),
        .wireframe = wireframe_,
        .show_grid = show_grid_,
        .show_axes = show_axes_,
    };
}

void Viewer::DrawPanel() {
    constexpr ImVec2 kPanelPosition{10.0F, 10.0F};
    ImGui::SetNextWindowPos(kPanelPosition, ImGuiCond_FirstUseEver);
    ImGui::Begin("gl-engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    const double average = stats_.Average().count();
    const std::string frame =
        std::format("frame {:.2f} ms avg, {:.2f} ms max ({:.0f} fps)", average,
                    stats_.Max().count(), average > 0.0 ? 1000.0 / average : 0.0);
    ImGui::TextUnformatted(frame.c_str());
    ImGui::Separator();

    bool fly = mode_ == CameraMode::kFly;
    if (ImGui::Checkbox("fly camera (Tab)", &fly)) {
        ToggleMode();
    }
    ImGui::Checkbox("wireframe", &wireframe_);
    ImGui::Checkbox("grid", &show_grid_);
    ImGui::Checkbox("axes", &show_axes_);
    ImGui::SliderAngle("light azimuth", &light_azimuth_, -180.0F, 180.0F);
    ImGui::SliderAngle("light elevation", &light_elevation_, -89.0F, 89.0F);

    ImGui::End();
}

}  // namespace gl_engine::viewer
