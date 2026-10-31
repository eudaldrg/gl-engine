#include "gl_engine/viewer/camera.h"

#include <gtest/gtest.h>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace gl_engine::viewer {
namespace {

constexpr float kTolerance = 1e-4F;

TEST(ForwardTest, ZeroAnglesLookDownNegativeZ) {
    const glm::vec3 forward = Forward(0.0F, 0.0F);
    EXPECT_NEAR(forward.x, 0.0F, kTolerance);
    EXPECT_NEAR(forward.y, 0.0F, kTolerance);
    EXPECT_NEAR(forward.z, -1.0F, kTolerance);
}

TEST(OrbitCameraTest, RotatingKeepsTheDistanceToTheTarget) {
    OrbitCamera camera;
    camera.target = {1.0F, 2.0F, 3.0F};
    camera.distance = 4.0F;
    for (int i = 0; i < 50; ++i) {
        camera.Rotate(0.3F, 0.11F);
        EXPECT_NEAR(glm::distance(camera.Position(), camera.target), 4.0F, kTolerance);
    }
}

TEST(OrbitCameraTest, PitchStopsShortOfVertical) {
    OrbitCamera camera;
    camera.Rotate(0.0F, 10.0F);
    EXPECT_LT(camera.pitch, glm::half_pi<float>());
    camera.Rotate(0.0F, -20.0F);
    EXPECT_GT(camera.pitch, -glm::half_pi<float>());
}

TEST(OrbitCameraTest, ZoomScalesTheDistanceAndNeverReachesTheTarget) {
    OrbitCamera camera;
    camera.distance = 10.0F;
    camera.Zoom(1.0F);
    EXPECT_LT(camera.distance, 10.0F);
    camera.Zoom(1000.0F);
    EXPECT_GT(camera.distance, 0.0F);
}

TEST(OrbitCameraTest, PanMovesTheTargetAcrossTheView) {
    OrbitCamera camera;
    camera.distance = 2.0F;
    camera.Pan({1.0F, 0.0F});
    // Looking down -z, right is +x; the pan is a fraction of the distance.
    EXPECT_NEAR(camera.target.x, 2.0F, kTolerance);
    EXPECT_NEAR(camera.target.z, 0.0F, kTolerance);
}

TEST(FlyCameraTest, MovesAlongForward) {
    FlyCamera camera;
    camera.yaw = 0.7F;
    camera.pitch = -0.3F;
    camera.Move({0.0F, 0.0F, 1.0F}, 2.5F);
    const glm::vec3 expected = 2.5F * Forward(0.7F, -0.3F);
    EXPECT_NEAR(glm::distance(camera.position, expected), 0.0F, kTolerance);
}

TEST(FlyCameraTest, NoDirectionMeansNoMovement) {
    FlyCamera camera;
    camera.Move({0.0F, 0.0F, 0.0F}, 5.0F);
    EXPECT_EQ(camera.position, glm::vec3{0.0F});
}

}  // namespace
}  // namespace gl_engine::viewer
