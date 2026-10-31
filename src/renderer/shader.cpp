#include "shader.h"

#include <format>
#include <fstream>
#include <iterator>
#include <string>

namespace gl_engine::renderer {

namespace {

base::Result<std::string> ReadFile(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    if (!in) {
        return std::unexpected{
            base::Error{base::ErrorCode::kIo, std::format("cannot read {}", path.string())}};
    }
    return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

// The driver's info log, without the terminating NUL.
template <class GetIv, class GetLog>
std::string InfoLog(GLuint id, GetIv get_iv, GetLog get_log) {
    GLint length = 0;
    get_iv(id, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    get_log(id, length, nullptr, log.data());
    log.resize(static_cast<std::size_t>(length) - 1);
    return log;
}

base::Result<Shader> CompileShader(GLenum type, const std::filesystem::path& path) {
    auto source = ReadFile(path);
    if (!source) {
        return std::unexpected{std::move(source.error())};
    }
    Shader shader = CreateShader(type);
    const GLchar* text = source->c_str();
    const auto length = static_cast<GLint>(source->size());
    glShaderSource(shader.Get(), 1, &text, &length);
    glCompileShader(shader.Get());

    GLint ok = GL_FALSE;
    glGetShaderiv(shader.Get(), GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        return std::unexpected{
            base::Error{base::ErrorCode::kParse,
                        std::format("{} failed to compile:\n{}", path.string(),
                                    InfoLog(shader.Get(), glGetShaderiv, glGetShaderInfoLog))}};
    }
    return shader;
}

}  // namespace

base::Result<Program> BuildProgram(const std::filesystem::path& vertex,
                                   const std::filesystem::path& fragment) {
    auto vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex);
    if (!vertex_shader) {
        return std::unexpected{std::move(vertex_shader.error())};
    }
    auto fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment);
    if (!fragment_shader) {
        return std::unexpected{std::move(fragment_shader.error())};
    }

    Program program = CreateProgram();
    glAttachShader(program.Get(), vertex_shader->Get());
    glAttachShader(program.Get(), fragment_shader->Get());
    glLinkProgram(program.Get());
    // The program keeps the compiled code; the shader objects are no longer needed.
    glDetachShader(program.Get(), vertex_shader->Get());
    glDetachShader(program.Get(), fragment_shader->Get());

    GLint ok = GL_FALSE;
    glGetProgramiv(program.Get(), GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        return std::unexpected{base::Error{
            base::ErrorCode::kParse,
            std::format("{} + {} failed to link:\n{}", vertex.string(), fragment.string(),
                        InfoLog(program.Get(), glGetProgramiv, glGetProgramInfoLog))}};
    }
    return program;
}

}  // namespace gl_engine::renderer
