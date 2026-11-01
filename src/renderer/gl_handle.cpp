#include "gl_engine/renderer/gl_handle.h"

namespace gl_engine::renderer {

void BufferTag::Delete(GLuint id) {
    glDeleteBuffers(1, &id);
}
void TextureTag::Delete(GLuint id) {
    glDeleteTextures(1, &id);
}
void VertexArrayTag::Delete(GLuint id) {
    glDeleteVertexArrays(1, &id);
}
void FramebufferTag::Delete(GLuint id) {
    glDeleteFramebuffers(1, &id);
}
void ShaderTag::Delete(GLuint id) {
    glDeleteShader(id);
}
void ProgramTag::Delete(GLuint id) {
    glDeleteProgram(id);
}

Buffer CreateBuffer() {
    GLuint id = 0;
    glCreateBuffers(1, &id);
    return Buffer{id};
}

Texture CreateTexture(GLenum target) {
    GLuint id = 0;
    glCreateTextures(target, 1, &id);
    return Texture{id};
}

VertexArray CreateVertexArray() {
    GLuint id = 0;
    glCreateVertexArrays(1, &id);
    return VertexArray{id};
}

Framebuffer CreateFramebuffer() {
    GLuint id = 0;
    glCreateFramebuffers(1, &id);
    return Framebuffer{id};
}

Shader CreateShader(GLenum type) {
    return Shader{glCreateShader(type)};
}

Program CreateProgram() {
    return Program{glCreateProgram()};
}

}  // namespace gl_engine::renderer
