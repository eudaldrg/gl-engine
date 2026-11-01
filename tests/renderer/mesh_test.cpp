#include "gl_engine/renderer/mesh.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <glm/geometric.hpp>

namespace gl_engine::renderer {
namespace {

constexpr float kTolerance = 1e-5F;

void ExpectUnitNormals(const MeshData& mesh) {
    for (const Vertex& vertex : mesh.vertices) {
        EXPECT_NEAR(glm::length(vertex.normal), 1.0F, kTolerance);
    }
}

// Every triangle is counter-clockwise seen from outside: its face normal agrees with the vertex
// normals.
void ExpectOutwardWinding(const MeshData& mesh) {
    ASSERT_EQ(mesh.indices.size() % 3, 0U);
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex& a = mesh.vertices.at(mesh.indices[i]);
        const Vertex& b = mesh.vertices.at(mesh.indices[i + 1]);
        const Vertex& c = mesh.vertices.at(mesh.indices[i + 2]);
        const glm::vec3 face = glm::cross(b.position - a.position, c.position - a.position);
        EXPECT_GT(glm::dot(face, a.normal), 0.0F) << "triangle " << i / 3;
    }
}

class IcosphereTest : public ::testing::TestWithParam<int> {};

TEST_P(IcosphereTest, CountsFollowTheSubdivisionLevel) {
    const int level = GetParam();
    const MeshData mesh = MakeIcosphere(level);
    const std::size_t faces = std::size_t{20} << (2 * static_cast<std::size_t>(level));
    EXPECT_EQ(mesh.vertices.size(), faces / 2 + 2);  // 10 * 4^n + 2
    EXPECT_EQ(mesh.indices.size(), faces * 3);
}

TEST_P(IcosphereTest, VerticesLieOnTheUnitSphere) {
    const MeshData mesh = MakeIcosphere(GetParam());
    ExpectUnitNormals(mesh);
    for (const Vertex& vertex : mesh.vertices) {
        EXPECT_NEAR(glm::length(vertex.position), 1.0F, kTolerance);
    }
}

TEST_P(IcosphereTest, TrianglesFaceOutward) {
    ExpectOutwardWinding(MakeIcosphere(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(Levels, IcosphereTest, ::testing::Values(0, 1, 3));

TEST(CubeTest, HasFourVerticesAndTwoTrianglesPerFace) {
    const MeshData mesh = MakeCube();
    EXPECT_EQ(mesh.vertices.size(), 24U);
    EXPECT_EQ(mesh.indices.size(), 36U);
}

TEST(CubeTest, NormalsAreUnitAndPerFace) {
    const MeshData mesh = MakeCube();
    ExpectUnitNormals(mesh);
    for (const Vertex& vertex : mesh.vertices) {
        // On a unit cube, the face normal is the axis the vertex sits furthest along.
        EXPECT_NEAR(glm::dot(vertex.position, vertex.normal), 0.5F, kTolerance);
    }
}

TEST(CubeTest, TrianglesFaceOutward) {
    ExpectOutwardWinding(MakeCube());
}

}  // namespace
}  // namespace gl_engine::renderer
