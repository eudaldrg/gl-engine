#include "gl_engine/base/log.h"

#include <atomic>
#include <cstdio>
#include <mutex>
#include <print>
#include <utility>

namespace gl_engine::base {

namespace {

struct LoggerState {
    std::atomic<Level> level{Level::kInfo};
    std::mutex sink_mutex;
    LogSink sink;
};

LoggerState& State() {
    static LoggerState state;
    return state;
}

void WriteToStderr(Level level, std::string_view message) {
    std::println(stderr, "[{}] {}", ToString(level), message);
}

}  // namespace

std::string_view ToString(Level level) {
    switch (level) {
        case Level::kTrace:
            return "trace";
        case Level::kDebug:
            return "debug";
        case Level::kInfo:
            return "info";
        case Level::kWarn:
            return "warn";
        case Level::kError:
            return "error";
    }
    return "unknown";
}

void SetLevel(Level level) {
    State().level.store(level, std::memory_order_relaxed);
}

Level GetLevel() {
    return State().level.load(std::memory_order_relaxed);
}

void SetSink(LogSink sink) {
    LoggerState& state = State();
    const std::scoped_lock lock{state.sink_mutex};
    state.sink = std::move(sink);
}

bool IsEnabled(Level level) {
    return level >= GetLevel();
}

void Write(Level level, std::string_view message) {
    if (!IsEnabled(level)) {
        return;
    }
    LoggerState& state = State();
    const std::scoped_lock lock{state.sink_mutex};
    if (state.sink) {
        state.sink(level, message);
    } else {
        WriteToStderr(level, message);
    }
}

}  // namespace gl_engine::base
