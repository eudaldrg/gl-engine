// Calls back when a watched file changes, appears or disappears.
//
// Callers only register paths; how changes are detected is an implementation detail. Today it
// compares modification times, at most once per interval, from Poll(). OS notifications can
// replace that later without changing callers.
#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>

namespace gl_engine::base {

enum class WatchId : std::uint64_t {};

class FileWatcher {
  public:
    using Callback = std::function<void(const std::filesystem::path&)>;
    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::milliseconds kDefaultInterval{250};

    // `interval` is the minimum time between two checks; tests pass zero to check on every Poll.
    explicit FileWatcher(Clock::duration interval = kDefaultInterval);

    // The path does not have to exist yet: its creation is reported as a change.
    WatchId Watch(std::filesystem::path path, Callback callback);
    // Unknown ids are ignored. Safe to call from inside a callback.
    void Unwatch(WatchId watch_id);

    // The host calls this once per frame. Callbacks run on the calling thread, from here only.
    void Poll();

  private:
    struct Entry {
        std::filesystem::path path;
        Callback callback;
        std::optional<std::filesystem::file_time_type> last_write;  // empty: file missing
    };

    static std::optional<std::filesystem::file_time_type> QueryWriteTime(
        const std::filesystem::path& path);

    Clock::duration interval_;
    std::optional<Clock::time_point> last_check_;
    std::map<WatchId, Entry> entries_;
    std::uint64_t next_id_ = 1;
};

}  // namespace gl_engine::base
