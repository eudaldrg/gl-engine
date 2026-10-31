#include "gl_engine/base/file_watcher.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace gl_engine::base {
namespace {

namespace fs = std::filesystem;
using std::chrono_literals::operator""h;
using std::chrono_literals::operator""s;

class FileWatcherTest : public ::testing::Test {
  protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir_ = fs::temp_directory_path() / (std::string{"gl_engine_file_watcher_"} + info->name());
        fs::remove_all(dir_);
        fs::create_directories(dir_);
    }
    void TearDown() override {
        std::error_code ignored;
        fs::remove_all(dir_, ignored);
    }

    static void WriteFile(const fs::path& path, const std::string& text) {
        std::ofstream out{path};
        out << text;
    }

    // Moves the modification time forward explicitly; two writes in a row can share a timestamp
    // on file systems with coarse resolution.
    static void Touch(const fs::path& path) {
        fs::last_write_time(path, fs::last_write_time(path) + 2s);
    }

    FileWatcher::Callback Recorder() {
        return [this](const fs::path& path) { reported_.push_back(path); };
    }

    fs::path dir_;
    std::vector<fs::path> reported_;
};

TEST_F(FileWatcherTest, ReportsNothingWhenUnchanged) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    watcher.Watch(file, Recorder());
    watcher.Poll();
    watcher.Poll();
    EXPECT_TRUE(reported_.empty());
}

TEST_F(FileWatcherTest, ReportsAModifiedFileOnce) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    watcher.Watch(file, Recorder());
    Touch(file);
    watcher.Poll();
    watcher.Poll();
    ASSERT_EQ(reported_.size(), 1U);
    EXPECT_EQ(reported_[0], file);
}

TEST_F(FileWatcherTest, ReportsCreationAfterMissing) {
    const fs::path file = dir_ / "later.glsl";
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    watcher.Watch(file, Recorder());
    watcher.Poll();
    EXPECT_TRUE(reported_.empty());
    WriteFile(file, "now");
    watcher.Poll();
    EXPECT_EQ(reported_.size(), 1U);
}

TEST_F(FileWatcherTest, ReportsDeletion) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    watcher.Watch(file, Recorder());
    fs::remove(file);
    watcher.Poll();
    watcher.Poll();
    EXPECT_EQ(reported_.size(), 1U);
}

TEST_F(FileWatcherTest, UnwatchStopsReports) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    const WatchId watch_id = watcher.Watch(file, Recorder());
    watcher.Unwatch(watch_id);
    Touch(file);
    watcher.Poll();
    EXPECT_TRUE(reported_.empty());
}

TEST_F(FileWatcherTest, CallbackMayUnwatchItself) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{FileWatcher::Clock::duration::zero()};
    WatchId watch_id{};
    watch_id = watcher.Watch(file, [&](const fs::path& path) {
        reported_.push_back(path);
        watcher.Unwatch(watch_id);
    });
    Touch(file);
    watcher.Poll();
    Touch(file);
    watcher.Poll();
    EXPECT_EQ(reported_.size(), 1U);
}

TEST_F(FileWatcherTest, ChecksAtMostOncePerInterval) {
    const fs::path file = dir_ / "a.glsl";
    WriteFile(file, "one");
    FileWatcher watcher{1h};
    watcher.Watch(file, Recorder());
    watcher.Poll();  // the first poll checks
    Touch(file);
    watcher.Poll();  // within the interval: no check
    EXPECT_TRUE(reported_.empty());
}

}  // namespace
}  // namespace gl_engine::base
