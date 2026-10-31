#include "gl_engine/viewer/viewer.h"

#include <gtest/gtest.h>

#include <chrono>
#include <glm/geometric.hpp>

namespace gl_engine::viewer {
namespace {

using std::chrono_literals::operator""ms;
constexpr float kTolerance = 1e-4F;

TEST(ViewerTest, ResizeUpdatesTheAspect) {
    Viewer viewer{800, 600};
    EXPECT_NEAR(viewer.Aspect(), 800.0F / 600.0F, kTolerance);
    viewer.HandleEvent(Resize{.width = 1000, .height = 500});
    EXPECT_NEAR(viewer.Aspect(), 2.0F, kTolerance);
    EXPECT_NEAR(viewer.BuildView().projection[1][1] / viewer.BuildView().projection[0][0], 2.0F,
                kTolerance);
}

TEST(ViewerTest, AMinimisedWindowKeepsTheLastAspect) {
    Viewer viewer{800, 400};
    viewer.HandleEvent(Resize{.width = 0, .height = 0});
    EXPECT_NEAR(viewer.Aspect(), 2.0F, kTolerance);
}

TEST(ViewerTest, LeftDragOrbitsAtAFixedDistance) {
    Viewer viewer{800, 600};
    const OrbitCamera before = viewer.Orbit();
    viewer.HandleEvent(MouseMove{.position = {100.0F, 100.0F}});
    viewer.HandleEvent(MouseButton{.button = Button::kLeft, .pressed = true});
    viewer.HandleEvent(MouseMove{.position = {160.0F, 130.0F}});
    viewer.HandleEvent(MouseButton{.button = Button::kLeft, .pressed = false});
    EXPECT_NE(viewer.Orbit().yaw, before.yaw);
    EXPECT_NE(viewer.Orbit().pitch, before.pitch);
    EXPECT_NEAR(glm::distance(viewer.Orbit().Position(), viewer.Orbit().target), before.distance,
                kTolerance);
}

TEST(ViewerTest, MovingWithoutAButtonDoesNotTurn) {
    Viewer viewer{800, 600};
    const OrbitCamera before = viewer.Orbit();
    viewer.HandleEvent(MouseMove{.position = {100.0F, 100.0F}});
    viewer.HandleEvent(MouseMove{.position = {300.0F, 50.0F}});
    EXPECT_EQ(viewer.Orbit().yaw, before.yaw);
    EXPECT_EQ(viewer.Orbit().pitch, before.pitch);
}

TEST(ViewerTest, TabSwitchesToFlyFromTheOrbitPose) {
    Viewer viewer{800, 600};
    viewer.HandleEvent(Key{.key = KeyCode::kTab, .pressed = true, .mods = {}});
    ASSERT_EQ(viewer.Mode(), CameraMode::kFly);
    EXPECT_NEAR(glm::distance(viewer.Fly().position, viewer.Orbit().Position()), 0.0F, kTolerance);
    EXPECT_EQ(viewer.Fly().yaw, viewer.Orbit().yaw);
    EXPECT_EQ(viewer.Fly().pitch, viewer.Orbit().pitch);
}

TEST(ViewerTest, HoldingWFliesForwardInProportionToFrameTime) {
    Viewer viewer{800, 600};
    viewer.HandleEvent(Key{.key = KeyCode::kTab, .pressed = true, .mods = {}});
    const glm::vec3 start = viewer.Fly().position;
    const glm::vec3 forward = Forward(viewer.Fly().yaw, viewer.Fly().pitch);

    viewer.HandleEvent(Key{.key = KeyCode::kW, .pressed = true, .mods = {}});
    viewer.Update(100ms);
    const float first = glm::distance(viewer.Fly().position, start);
    viewer.Update(200ms);
    const float total = glm::distance(viewer.Fly().position, start);
    EXPECT_GT(first, 0.0F);
    EXPECT_NEAR(total, 3.0F * first, kTolerance);
    EXPECT_NEAR(glm::dot(glm::normalize(viewer.Fly().position - start), forward), 1.0F, kTolerance);

    viewer.HandleEvent(Key{.key = KeyCode::kW, .pressed = false, .mods = {}});
    const glm::vec3 stopped = viewer.Fly().position;
    viewer.Update(100ms);
    EXPECT_EQ(viewer.Fly().position, stopped);
}

TEST(ViewerTest, UpdateRecordsFrameTimes) {
    Viewer viewer{800, 600};
    viewer.Update(10ms);
    viewer.Update(30ms);
    EXPECT_DOUBLE_EQ(viewer.Stats().Average().count(), 20.0);
    EXPECT_DOUBLE_EQ(viewer.Stats().Max().count(), 30.0);
}

}  // namespace
}  // namespace gl_engine::viewer
