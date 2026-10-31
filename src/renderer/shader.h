// Builds GL programs from GLSL files. Internal to the renderer.
#pragma once

#include <filesystem>

#include "gl_engine/base/error.h"
#include "gl_engine/renderer/gl_handle.h"

namespace gl_engine::renderer {

// Compiles and links a vertex and a fragment shader. Read failures are kIo; compile and link
// failures are kParse, with the driver's log in the message.
[[nodiscard]] base::Result<Program> BuildProgram(const std::filesystem::path& vertex,
                                                 const std::filesystem::path& fragment);

}  // namespace gl_engine::renderer
