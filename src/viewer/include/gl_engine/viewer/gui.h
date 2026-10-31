// The Dear ImGui context and its OpenGL renderer. The host adds its platform backend (for GLFW,
// imgui_impl_glfw) after Create and shuts it down before destroying the Gui.
#pragma once

#include <memory>

#include "gl_engine/base/error.h"
#include "gl_engine/renderer/renderer.h"

namespace gl_engine::viewer {

class Gui {
  public:
    // Needs a current GL context. No .ini file is read or written.
    [[nodiscard]] static base::Result<std::unique_ptr<Gui>> Create();
    ~Gui();

    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;

    // Starts a frame, after the host's platform backend has started its own.
    void NewFrame();
    // Draws the frame's widgets on top of whatever the target holds.
    void Render(const renderer::RenderTarget& target);

  private:
    Gui() = default;
};

}  // namespace gl_engine::viewer
