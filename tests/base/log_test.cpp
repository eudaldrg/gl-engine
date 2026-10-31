#include "gl_engine/base/log.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace gl_engine::base {
namespace {

class LogTest : public ::testing::Test {
  protected:
    void SetUp() override {
        SetLevel(Level::kInfo);
        SetSink([this](Level level, std::string_view message) {
            messages_.emplace_back(level, std::string{message});
        });
    }
    void TearDown() override {
        SetSink({});
        SetLevel(Level::kInfo);
    }

    std::vector<std::pair<Level, std::string>> messages_;
};

TEST_F(LogTest, FormatsArguments) {
    Log(Level::kWarn, "{} of {}", 3, "four");
    ASSERT_EQ(messages_.size(), 1U);
    EXPECT_EQ(messages_[0].first, Level::kWarn);
    EXPECT_EQ(messages_[0].second, "3 of four");
}

TEST_F(LogTest, DropsMessagesBelowTheLevel) {
    SetLevel(Level::kWarn);
    Log(Level::kDebug, "dropped");
    Log(Level::kInfo, "dropped");
    Log(Level::kWarn, "kept");
    Log(Level::kError, "kept");
    EXPECT_EQ(messages_.size(), 2U);
    EXPECT_FALSE(IsEnabled(Level::kInfo));
    EXPECT_TRUE(IsEnabled(Level::kError));
}

TEST_F(LogTest, TraceNeedsTheLowestLevel) {
    Log(Level::kTrace, "dropped at info");
    SetLevel(Level::kTrace);
    Log(Level::kTrace, "kept");
    ASSERT_EQ(messages_.size(), 1U);
    EXPECT_EQ(messages_[0].second, "kept");
}

TEST_F(LogTest, ReplacingTheSinkRedirectsMessages) {
    std::vector<std::string> other;
    SetSink([&other](Level /*level*/, std::string_view message) { other.emplace_back(message); });
    Log(Level::kError, "to the new sink");
    EXPECT_TRUE(messages_.empty());
    ASSERT_EQ(other.size(), 1U);
    EXPECT_EQ(other[0], "to the new sink");
    SetSink({});
}

}  // namespace
}  // namespace gl_engine::base
