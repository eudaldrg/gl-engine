#include "gl_engine/renderer/renderer.h"

#include <cstddef>
#include <cstdlib>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <string_view>
#include <utility>
#include <vector>

#include "gl_engine/base/log.h"
#include "gl_engine/renderer/mesh.h"
#include "shader.h"

namespace gl_engine::renderer {

namespace {

using base::Error;
using base::ErrorCode;
using base::Level;

constexpr int kRequiredMajor = 4;
constexpr int kRequiredMinor = 5;

constexpr int kSphereSubdivisions = 3;
constexpr int kGridHalfExtent = 5;  // grid lines from -5 to 5 on x and z, one unit apart
constexpr float kAxisLength = 1.0F;

// Scene layout: y is up, the grid lies on y = 0, both shapes rest on it either side of the axes.
const glm::mat4 kSphereModel =
    glm::scale(glm::translate(glm::mat4{1.0F}, {2.0F, 0.5F, 0.0F}), glm::vec3{0.5F});
const glm::mat4 kCubeModel = glm::translate(glm::mat4{1.0F}, {-2.0F, 0.5F, 0.0F});
constexpr glm::vec3 kSphereColor{0.8F, 0.35F, 0.2F};
constexpr glm::vec3 kCubeColor{0.25F, 0.5F, 0.8F};
constexpr glm::vec3 kGridColor{0.35F, 0.35F, 0.38F};
constexpr glm::vec3 kClearColor{0.1F, 0.1F, 0.12F};

// Explicit uniform locations, matching the layout qualifiers in shaders/.
constexpr GLint kMeshModel = 0;
constexpr GLint kMeshViewProjection = 1;
constexpr GLint kMeshCameraPosition = 2;
constexpr GLint kMeshLightDirection = 3;
constexpr GLint kMeshColor = 4;
constexpr GLint kLineViewProjection = 0;

struct LineVertex {
    glm::vec3 position;
    glm::vec3 color;
};

std::string_view GlString(GLenum name) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): GL returns text as GLubyte*.
    return reinterpret_cast<const char*>(glGetString(name));
}

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

void GLAD_API_PTR OnDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity,
                                 GLsizei length, const GLchar* message, const void* /*user*/) {
    const std::string_view text = length >= 0
                                      ? std::string_view{message, static_cast<std::size_t>(length)}
                                      : std::string_view{message};
    // Compile and link failures come back from ReloadShader as an Error with the same log; some
    // drivers also report them here, so they are kept out of the log at its default level.
    const bool from_compiler = source == GL_DEBUG_SOURCE_SHADER_COMPILER;
    base::Log(from_compiler ? Level::kDebug : LevelFor(severity), "GL [{}] {}", id, text);
#ifndef NDEBUG
    // A GL API error is a broken invariant in our code: stop where it happened (ADR 0002).
    if (source == GL_DEBUG_SOURCE_API && type == GL_DEBUG_TYPE_ERROR) {
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

template <class T>
Buffer CreateStaticBuffer(const std::vector<T>& data) {
    Buffer buffer = CreateBuffer();
    glNamedBufferStorage(buffer.Get(), static_cast<GLsizeiptr>(data.size() * sizeof(T)),
                         data.data(), 0);
    return buffer;
}

// Both vertex types are two vec3 attributes: location 0 at offset 0, location 1 right after.
template <class VertexType>
void BindTwoVec3Attributes(GLuint vertex_array, GLuint buffer) {
    static_assert(sizeof(VertexType) == 2 * sizeof(glm::vec3));
    glVertexArrayVertexBuffer(vertex_array, 0, buffer, 0, sizeof(VertexType));
    for (const GLuint location : {0U, 1U}) {
        glEnableVertexArrayAttrib(vertex_array, location);
        glVertexArrayAttribFormat(vertex_array, location, 3, GL_FLOAT, GL_FALSE,
                                  location * static_cast<GLuint>(sizeof(glm::vec3)));
        glVertexArrayAttribBinding(vertex_array, location, 0);
    }
}

std::vector<LineVertex> MakeAxes() {
    const glm::vec3 x{1, 0, 0};
    const glm::vec3 y{0, 1, 0};
    const glm::vec3 z{0, 0, 1};
    const glm::vec3 origin{0, 0, 0};
    // Each axis is drawn in the colour of its own direction: x red, y green, z blue.
    return {{origin, x},          {kAxisLength * x, x}, {origin, y},
            {kAxisLength * y, y}, {origin, z},          {kAxisLength * z, z}};
}

// The grid's x = 0 and z = 0 lines run under the x and z axes. Two lines on the same pixels get
// slightly different depths from rounding and would hide each other in patches, so the grid never
// overlaps the axes: the spans under them come first (kGridUnderAxesVertexCount vertices), and the
// renderer draws those only while the axes are hidden.
constexpr GLsizei kGridUnderAxesVertexCount = 4;

std::vector<LineVertex> MakeGrid() {
    const auto extent = static_cast<float>(kGridHalfExtent);
    std::vector<LineVertex> lines = {{{0, 0, 0}, kGridColor},
                                     {{kAxisLength, 0, 0}, kGridColor},
                                     {{0, 0, 0}, kGridColor},
                                     {{0, 0, kAxisLength}, kGridColor}};
    auto add = [&lines](glm::vec3 from, glm::vec3 to) {
        lines.push_back({from, kGridColor});
        lines.push_back({to, kGridColor});
    };
    for (int i = -kGridHalfExtent; i <= kGridHalfExtent; ++i) {
        const auto offset = static_cast<float>(i);
        if (i == 0) {
            add({0, 0, -extent}, {0, 0, 0});
            add({0, 0, kAxisLength}, {0, 0, extent});
            add({-extent, 0, 0}, {0, 0, 0});
            add({kAxisLength, 0, 0}, {extent, 0, 0});
        } else {
            add({offset, 0, -extent}, {offset, 0, extent});
            add({-extent, 0, offset}, {extent, 0, offset});
        }
    }
    return lines;
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
    base::Log(Level::kInfo, "OpenGL {} on {}", GlString(GL_VERSION), GlString(GL_RENDERER));

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);  // report on the offending call's stack
    glDebugMessageCallback(OnDebugMessage, nullptr);

    Renderer renderer;
    const std::filesystem::path& dir = config.shader_dir;
    renderer.mesh_program_ = {
        .vertex = dir / "mesh.vert", .fragment = dir / "mesh.frag", .program = {}};
    renderer.line_program_ = {
        .vertex = dir / "line.vert", .fragment = dir / "line.frag", .program = {}};

    for (ShaderProgram* entry : renderer.Programs()) {
        auto program = BuildProgram(entry->vertex, entry->fragment);
        if (!program) {
            return std::unexpected{std::move(program.error())};
        }
        entry->program = std::move(*program);
        renderer.shader_files_.push_back(entry->vertex);
        renderer.shader_files_.push_back(entry->fragment);
    }

    const auto upload = [](const MeshData& mesh) {
        GpuMesh gpu{.vertex_array = CreateVertexArray(),
                    .vertices = CreateStaticBuffer(mesh.vertices),
                    .indices = CreateStaticBuffer(mesh.indices),
                    .index_count = static_cast<GLsizei>(mesh.indices.size())};
        BindTwoVec3Attributes<Vertex>(gpu.vertex_array.Get(), gpu.vertices.Get());
        glVertexArrayElementBuffer(gpu.vertex_array.Get(), gpu.indices.Get());
        return gpu;
    };
    renderer.sphere_ = upload(MakeIcosphere(kSphereSubdivisions));
    renderer.cube_ = upload(MakeCube());

    std::vector<LineVertex> lines = MakeAxes();
    renderer.axes_vertex_count_ = static_cast<GLsizei>(lines.size());
    const std::vector<LineVertex> grid = MakeGrid();
    renderer.grid_vertex_count_ = static_cast<GLsizei>(grid.size());
    lines.insert(lines.end(), grid.begin(), grid.end());
    renderer.lines_ = CreateVertexArray();
    renderer.line_vertices_ = CreateStaticBuffer(lines);
    BindTwoVec3Attributes<LineVertex>(renderer.lines_.Get(), renderer.line_vertices_.Get());

    return renderer;
}

void Renderer::Render(const RenderTarget& target, const View& view) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glViewport(0, 0, target.width, target.height);
    glClearColor(kClearColor.x, kClearColor.y, kClearColor.z, 1.0F);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    const glm::mat4 view_projection = view.projection * view.view;
    const glm::vec3 camera_position{glm::inverse(view.view)[3]};

    glUseProgram(mesh_program_.program.Get());
    glUniformMatrix4fv(kMeshViewProjection, 1, GL_FALSE, glm::value_ptr(view_projection));
    glUniform3fv(kMeshCameraPosition, 1, glm::value_ptr(camera_position));
    glUniform3fv(kMeshLightDirection, 1, glm::value_ptr(view.light_direction));
    glPolygonMode(GL_FRONT_AND_BACK, view.wireframe ? GL_LINE : GL_FILL);
    DrawMesh(sphere_, kSphereModel, kSphereColor);
    DrawMesh(cube_, kCubeModel, kCubeColor);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(line_program_.program.Get());
    glUniformMatrix4fv(kLineViewProjection, 1, GL_FALSE, glm::value_ptr(view_projection));
    glBindVertexArray(lines_.Get());
    if (view.show_grid) {
        const GLsizei skipped = view.show_axes ? kGridUnderAxesVertexCount : 0;
        glDrawArrays(GL_LINES, axes_vertex_count_ + skipped, grid_vertex_count_ - skipped);
    }
    if (view.show_axes) {
        glDrawArrays(GL_LINES, 0, axes_vertex_count_);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::DrawMesh(const GpuMesh& mesh, const glm::mat4& model, const glm::vec3& color) const {
    glUniformMatrix4fv(kMeshModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(kMeshColor, 1, glm::value_ptr(color));
    glBindVertexArray(mesh.vertex_array.Get());
    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
}

base::Result<void> Renderer::ReloadShader(const std::filesystem::path& path) {
    // Build everything first and swap only if all succeed, so a failure changes nothing.
    std::vector<std::pair<ShaderProgram*, Program>> rebuilt;
    for (ShaderProgram* entry : Programs()) {
        if (entry->vertex != path && entry->fragment != path) {
            continue;
        }
        auto program = BuildProgram(entry->vertex, entry->fragment);
        if (!program) {
            return std::unexpected{std::move(program.error())};
        }
        rebuilt.emplace_back(entry, std::move(*program));
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

}  // namespace gl_engine::renderer
