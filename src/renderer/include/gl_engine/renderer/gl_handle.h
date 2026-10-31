// Move-only owners for GL objects. Every handle must be destroyed while its context is current.
#pragma once

#include <glad/gl.h>

#include <utility>

namespace gl_engine::renderer {

// `Tag` names the object type and supplies `static void Delete(GLuint)`.
template <class Tag>
class GlHandle {
  public:
    GlHandle() = default;
    explicit GlHandle(GLuint id) : id_{id} {}
    ~GlHandle() {
        Reset();
    }

    GlHandle(const GlHandle&) = delete;
    GlHandle& operator=(const GlHandle&) = delete;
    GlHandle(GlHandle&& other) noexcept : id_{std::exchange(other.id_, 0)} {}
    GlHandle& operator=(GlHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            id_ = std::exchange(other.id_, 0);
        }
        return *this;
    }

    GLuint Get() const {
        return id_;
    }
    explicit operator bool() const {
        return id_ != 0;
    }

    // Deletes the object now; the handle is empty afterwards.
    void Reset() {
        if (id_ != 0) {
            Tag::Delete(id_);
            id_ = 0;
        }
    }

  private:
    GLuint id_ = 0;
};

struct BufferTag {
    static void Delete(GLuint id);
};
struct TextureTag {
    static void Delete(GLuint id);
};
struct VertexArrayTag {
    static void Delete(GLuint id);
};
struct FramebufferTag {
    static void Delete(GLuint id);
};
struct ShaderTag {
    static void Delete(GLuint id);
};
struct ProgramTag {
    static void Delete(GLuint id);
};

using Buffer = GlHandle<BufferTag>;
using Texture = GlHandle<TextureTag>;
using VertexArray = GlHandle<VertexArrayTag>;
using Framebuffer = GlHandle<FramebufferTag>;
using Shader = GlHandle<ShaderTag>;
using Program = GlHandle<ProgramTag>;

// Factories through direct state access (glCreate*), so the objects exist before first bind.
Buffer CreateBuffer();
Texture CreateTexture(GLenum target);
VertexArray CreateVertexArray();
Framebuffer CreateFramebuffer();
Shader CreateShader(GLenum type);
Program CreateProgram();

}  // namespace gl_engine::renderer
