// Procedural meshes for the test scene, built on the CPU with no GL.
#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <vector>

namespace gl_engine::renderer {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// Indexed triangles, counter-clockwise when seen from outside.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Unit-radius sphere centred on the origin: an icosahedron whose faces are split into four,
// `subdivisions` times. Level n has 10 * 4^n + 2 vertices and 20 * 4^n triangles.
MeshData MakeIcosphere(int subdivisions);

// Unit cube centred on the origin, four vertices per face so each face has its own normal.
MeshData MakeCube();

}  // namespace gl_engine::renderer
