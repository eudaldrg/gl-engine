#include "gl_engine/renderer/mesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <map>
#include <utility>

namespace gl_engine::renderer {

namespace {

using Triangle = std::array<std::uint32_t, 3>;

// Returns the vertex halfway along the edge (a, b), pushed onto the unit sphere. Shared edges
// reuse the vertex the neighbouring triangle already created.
std::uint32_t Midpoint(std::uint32_t a, std::uint32_t b, std::vector<glm::vec3>& positions,
                       std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t>& cache) {
    const auto key = std::minmax(a, b);
    if (const auto found = cache.find(key); found != cache.end()) {
        return found->second;
    }
    const auto index = static_cast<std::uint32_t>(positions.size());
    positions.push_back(glm::normalize(positions[a] + positions[b]));
    cache.emplace(key, index);
    return index;
}

}  // namespace

MeshData MakeIcosphere(int subdivisions) {
    const float t = (1.0F + std::sqrt(5.0F)) / 2.0F;
    std::vector<glm::vec3> positions{
        {-1, t, 0},  {1, t, 0},  {-1, -t, 0}, {1, -t, 0}, {0, -1, t},  {0, 1, t},
        {0, -1, -t}, {0, 1, -t}, {t, 0, -1},  {t, 0, 1},  {-t, 0, -1}, {-t, 0, 1},
    };
    for (glm::vec3& position : positions) {
        position = glm::normalize(position);
    }
    std::vector<Triangle> triangles{
        {0, 11, 5},  {0, 5, 1},  {0, 1, 7},  {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
        {11, 10, 2}, {10, 7, 6}, {7, 1, 8},  {3, 9, 4},  {3, 4, 2},   {3, 2, 6}, {3, 6, 8},
        {3, 8, 9},   {4, 9, 5},  {2, 4, 11}, {6, 2, 10}, {8, 6, 7},   {9, 8, 1},
    };

    for (int level = 0; level < subdivisions; ++level) {
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> cache;
        std::vector<Triangle> finer;
        finer.reserve(triangles.size() * 4);
        for (const auto& [a, b, c] : triangles) {
            const std::uint32_t ab = Midpoint(a, b, positions, cache);
            const std::uint32_t bc = Midpoint(b, c, positions, cache);
            const std::uint32_t ca = Midpoint(c, a, positions, cache);
            finer.push_back({a, ab, ca});
            finer.push_back({b, bc, ab});
            finer.push_back({c, ca, bc});
            finer.push_back({ab, bc, ca});
        }
        triangles = std::move(finer);
    }

    MeshData mesh;
    mesh.vertices.reserve(positions.size());
    for (const glm::vec3& position : positions) {
        mesh.vertices.push_back({.position = position, .normal = position});
    }
    mesh.indices.reserve(triangles.size() * 3);
    for (const Triangle& triangle : triangles) {
        mesh.indices.insert(mesh.indices.end(), triangle.begin(), triangle.end());
    }
    return mesh;
}

MeshData MakeCube() {
    struct Face {
        glm::vec3 normal;
        glm::vec3 u;  // u x v == normal, so the corners below run counter-clockwise from outside
        glm::vec3 v;
    };
    constexpr std::array<Face, 6> kFaces{{
        {.normal = {1, 0, 0}, .u = {0, 1, 0}, .v = {0, 0, 1}},
        {.normal = {-1, 0, 0}, .u = {0, 0, 1}, .v = {0, 1, 0}},
        {.normal = {0, 1, 0}, .u = {0, 0, 1}, .v = {1, 0, 0}},
        {.normal = {0, -1, 0}, .u = {1, 0, 0}, .v = {0, 0, 1}},
        {.normal = {0, 0, 1}, .u = {1, 0, 0}, .v = {0, 1, 0}},
        {.normal = {0, 0, -1}, .u = {0, 1, 0}, .v = {1, 0, 0}},
    }};
    constexpr std::array<std::array<float, 2>, 4> kCorners{{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}};

    MeshData mesh;
    for (const Face& face : kFaces) {
        const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const auto& [s, r] : kCorners) {
            mesh.vertices.push_back({.position = 0.5F * (face.normal + s * face.u + r * face.v),
                                     .normal = face.normal});
        }
        mesh.indices.insert(mesh.indices.end(),
                            {first, first + 1, first + 2, first, first + 2, first + 3});
    }
    return mesh;
}

}  // namespace gl_engine::renderer
