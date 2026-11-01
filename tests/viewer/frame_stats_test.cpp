#include "gl_engine/viewer/frame_stats.h"

#include <gtest/gtest.h>

#include <chrono>

namespace gl_engine::viewer {
namespace {

using std::chrono_literals::operator""ms;

TEST(FrameStatsTest, EmptyIsZero) {
    const FrameStats stats;
    EXPECT_DOUBLE_EQ(stats.Average().count(), 0.0);
    EXPECT_DOUBLE_EQ(stats.Max().count(), 0.0);
}

TEST(FrameStatsTest, OldFramesLeaveTheWindow) {
    FrameStats stats{3};
    stats.Add(100ms);
    stats.Add(10ms);
    stats.Add(20ms);
    EXPECT_DOUBLE_EQ(stats.Max().count(), 100.0);
    stats.Add(30ms);  // the 100 ms frame drops out
    EXPECT_DOUBLE_EQ(stats.Average().count(), 20.0);
    EXPECT_DOUBLE_EQ(stats.Max().count(), 30.0);
}

}  // namespace
}  // namespace gl_engine::viewer
