#include "gl_engine/base/file_watcher.h"

#include <system_error>
#include <utility>
#include <vector>

namespace gl_engine::base {

FileWatcher::FileWatcher(Clock::duration interval) : interval_{interval} {}

WatchId FileWatcher::Watch(std::filesystem::path path, Callback callback) {
    const WatchId watch_id{next_id_++};
    auto last_write = QueryWriteTime(path);
    entries_.emplace(
        watch_id,
        Entry{.path = std::move(path), .callback = std::move(callback), .last_write = last_write});
    return watch_id;
}

void FileWatcher::Unwatch(WatchId watch_id) {
    entries_.erase(watch_id);
}

void FileWatcher::Poll() {
    const Clock::time_point now = Clock::now();
    if (last_check_ && now - *last_check_ < interval_) {
        return;
    }
    last_check_ = now;

    // Collect first and call afterwards, so a callback may watch or unwatch without
    // invalidating the iteration.
    std::vector<WatchId> changed;
    for (auto& [watch_id, entry] : entries_) {
        auto current = QueryWriteTime(entry.path);
        if (current != entry.last_write) {
            entry.last_write = current;
            changed.push_back(watch_id);
        }
    }
    for (const WatchId watch_id : changed) {
        const auto found = entries_.find(watch_id);
        if (found == entries_.end()) {
            continue;  // unwatched by an earlier callback
        }
        // Copies, so the callback may unwatch itself.
        const std::filesystem::path path = found->second.path;
        const Callback callback = found->second.callback;
        callback(path);
    }
}

std::optional<std::filesystem::file_time_type> FileWatcher::QueryWriteTime(
    const std::filesystem::path& path) {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(path, error);
    if (error) {
        return std::nullopt;
    }
    return time;
}

}  // namespace gl_engine::base
