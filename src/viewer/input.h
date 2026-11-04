// Host-neutral input events. Each host translates its own events into these; the viewer never
// sees GLFW or any other windowing library.
#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <variant>

namespace gl_engine::viewer {

enum class Button : std::uint8_t { kLeft, kRight, kMiddle };

// Only the keys the viewer reacts to; hosts map everything else to kUnknown.
enum class KeyCode : std::uint8_t { kUnknown, kW, kA, kS, kD, kQ, kE, kTab };

struct Modifiers {
    bool shift = false;
    bool control = false;
    bool alt = false;
};

struct MouseMove {
    glm::vec2 position{0.0F};  // window pixels, origin top left
};
struct MouseButton {
    Button button = Button::kLeft;
    bool pressed = false;
};
struct Scroll {
    float delta = 0.0F;  // positive away from the user
};
struct Key {
    KeyCode key = KeyCode::kUnknown;
    bool pressed = false;
    Modifiers mods;
};
struct Resize {
    int width = 0;
    int height = 0;
};

using InputEvent = std::variant<MouseMove, MouseButton, Scroll, Key, Resize>;

}  // namespace gl_engine::viewer
