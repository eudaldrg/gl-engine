#include "gl_engine/viewer/gui.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

namespace gl_engine::viewer {

base::Result<std::unique_ptr<Gui>> Gui::Create() {
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplOpenGL3_Init("#version 450 core")) {
        ImGui::DestroyContext();
        return std::unexpected{
            base::Error{base::ErrorCode::kGl, "could not start the ImGui OpenGL backend"}};
    }
    return std::unique_ptr<Gui>{new Gui};
}

Gui::~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void Gui::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void Gui::Render(const renderer::RenderTarget& target) {
    ImGui::Render();
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace gl_engine::viewer
