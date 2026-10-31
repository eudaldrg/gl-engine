// Draws the scene into a framebuffer the host provides. The host owns the window, the context and
// the loop (ADR 0003); the renderer never binds framebuffer 0 on its own or queries a window.
#pragma once

#include <glad/gl.h>

#include <array>
#include <filesystem>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <span>
#include <vector>

#include "gl_engine/base/error.h"
#include "gl_engine/renderer/gl_handle.h"

namespace gl_engine::renderer {

// What windowing libraries hand out to look up GL functions, e.g. glfwGetProcAddress.
using GlProc = void (*)();
using GlLoader = GlProc (*)(const char* name);

struct RendererConfig {
    // Holds the GLSL sources: the source tree in dev builds, a directory next to the binary
    // otherwise. The host decides.
    std::filesystem::path shader_dir;
};

struct RenderTarget {
    GLuint framebuffer = 0;
    int width = 0;
    int height = 0;
};

struct View {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    glm::vec3 light_direction{-0.4F, -1.0F, -0.6F};  // direction the light travels, world space
    bool wireframe = false;
    bool show_grid = true;
    bool show_axes = true;
};

class Renderer {
  public:
    // Needs a current GL 4.5 core context; anything older is ErrorCode::kUnsupported. Shaders that
    // fail to build are returned as their Error. Routes GL debug output to the log.
    [[nodiscard]] static base::Result<Renderer> Create(GlLoader loader, RendererConfig config);

    // Binds the target's framebuffer and draws into the whole of it.
    void Render(const RenderTarget& target, const View& view);

    // Every GLSL file the renderer reads, for the host to watch.
    std::span<const std::filesystem::path> ShaderFiles() const {
        return shader_files_;
    }

    // Rebuilds every program that uses `path`. On failure the previous programs stay in use and
    // the Error carries the compile or link log.
    [[nodiscard]] base::Result<void> ReloadShader(const std::filesystem::path& path);

  private:
    struct ShaderProgram {
        std::filesystem::path vertex;
        std::filesystem::path fragment;
        Program program;
    };

    struct GpuMesh {
        VertexArray vertex_array;
        Buffer vertices;
        Buffer indices;
        GLsizei index_count = 0;
    };

    Renderer() = default;
    // Every program, for loops that treat them alike. Built on demand: the Renderer moves.
    std::array<ShaderProgram*, 2> Programs() {
        return {&mesh_program_, &line_program_};
    }
    void DrawMesh(const GpuMesh& mesh, const glm::mat4& model, const glm::vec3& color) const;

    ShaderProgram mesh_program_;
    ShaderProgram line_program_;
    std::vector<std::filesystem::path> shader_files_;

    GpuMesh sphere_;
    GpuMesh cube_;
    // Debug lines: the axes first, then the grid.
    VertexArray lines_;
    Buffer line_vertices_;
    GLsizei axes_vertex_count_ = 0;
    GLsizei grid_vertex_count_ = 0;
};

}  // namespace gl_engine::renderer
