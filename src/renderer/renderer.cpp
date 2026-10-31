#include "gl_engine/renderer/renderer.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <string_view>
#include <utility>

#include "gl_engine/base/log.h"
#include "shader.h"

namespace gl_engine::renderer {

namespace {

using base::Error;
using base::ErrorCode;
using base::Level;

constexpr int kRequiredMajor = 4;
constexpr int kRequiredMinor = 5;

Level LevelFor(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            return Level::kError;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return Level::kWarn;
        case GL_DEBUG_SEVERITY_LOW:
            return Level::kInfo;
        default:
            return Level::kDebug;
    }
}

void GLAD_API_PTR OnDebugMessage(GLenum /*source*/, GLenum type, GLuint id, GLenum severity,
                                 GLsizei length, const GLchar* message, const void* /*user*/) {
    const std::string_view text = length >= 0
                                      ? std::string_view{message, static_cast<std::size_t>(length)}
                                      : std::string_view{message};
    base::Log(LevelFor(severity), "GL [{}] {}", id, text);
#ifndef NDEBUG
    // A GL error is a broken invariant in our code: stop where it happened (ADR 0002).
    if (type == GL_DEBUG_TYPE_ERROR) {
        std::abort();
    }
#else
    (void)type;
#endif
}

base::Result<void> CheckContext() {
    GLint major = 0;
    GLint minor = 0;
    GLint profile = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    const bool new_enough =
        major > kRequiredMajor || (major == kRequiredMajor && minor >= kRequiredMinor);
    const bool core = (static_cast<GLuint>(profile) & GL_CONTEXT_CORE_PROFILE_BIT) != 0U;
    if (!new_enough || !core) {
        return std::unexpected{
            Error{ErrorCode::kUnsupported,
                  std::format("needs an OpenGL {}.{} core context, got {}.{} {}", kRequiredMajor,
                              kRequiredMinor, major, minor, core ? "core" : "compatibility")}};
    }
    return {};
}

}  // namespace

base::Result<Renderer> Renderer::Create(GlLoader loader, RendererConfig config) {
    if (gladLoadGL(loader) == 0) {
        return std::unexpected{
            Error{ErrorCode::kUnsupported, "could not load OpenGL; is a context current?"}};
    }
    if (auto context = CheckContext(); !context) {
        return std::unexpected{std::move(context.error())};
    }
    base::Log(Level::kInfo, "OpenGL {} on {}",
              reinterpret_cast<const char*>(
                  glGetString(GL_VERSION)),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast):
                                             // GL returns strings as GLubyte*
              reinterpret_cast<const char*>(glGetString(
                  GL_RENDERER)));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): as above

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);  // report on the offending call's stack
    glDebugMessageCallback(OnDebugMessage, nullptr);

    Renderer renderer;
    const std::filesystem::path& dir = config.shader_dir;
    renderer.programs_[static_cast<std::size_t>(ProgramId::kLine)] = {
        .vertex = dir / "line.vert", .fragment = dir / "line.frag", .program = {}};

    for (ShaderProgram& entry : renderer.programs_) {
        auto program = BuildProgram(entry.vertex, entry.fragment);
        if (!program) {
            return std::unexpected{std::move(program.error())};
        }
        entry.program = std::move(*program);
        for (const auto* path : {&entry.vertex, &entry.fragment}) {
            if (std::ranges::find(renderer.shader_files_, *path) == renderer.shader_files_.end()) {
                renderer.shader_files_.push_back(*path);
            }
        }
    }
    return renderer;
}

void Renderer::Render(const RenderTarget& target, const View& /*view*/) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    glClearColor(0.1F, 0.1F, 0.12F, 1.0F);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

base::Result<void> Renderer::ReloadShader(const std::filesystem::path& path) {
    // Build everything first and swap only if all succeed, so a failure changes nothing.
    std::vector<std::pair<ShaderProgram*, Program>> rebuilt;
    for (ShaderProgram& entry : programs_) {
        if (entry.vertex != path && entry.fragment != path) {
            continue;
        }
        auto program = BuildProgram(entry.vertex, entry.fragment);
        if (!program) {
            return std::unexpected{std::move(program.error())};
        }
        rebuilt.emplace_back(&entry, std::move(*program));
    }
    if (rebuilt.empty()) {
        return std::unexpected{Error{
            ErrorCode::kIo, std::format("{} is not a shader of this renderer", path.string())}};
    }
    for (auto& [entry, program] : rebuilt) {
        entry->program = std::move(program);
    }
    base::Log(Level::kInfo, "reloaded {}", path.string());
    return {};
}

Program& Renderer::ProgramFor(ProgramId program_id) {
    return programs_[static_cast<std::size_t>(program_id)].program;
}

}  // namespace gl_engine::renderer
