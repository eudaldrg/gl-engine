// Process-wide logger: a runtime level filter and one replaceable sink.
#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string_view>
#include <utility>

namespace gl_engine::base {

enum class Level : std::uint8_t { kTrace, kDebug, kInfo, kWarn, kError };

std::string_view ToString(Level level);

using LogSink = std::function<void(Level, std::string_view)>;

// Messages below the level are dropped before formatting. Default: kInfo.
void SetLevel(Level level);
Level GetLevel();

// Replaces the sink; an empty function restores the default, which writes to stderr.
void SetSink(LogSink sink);

// True when a message at this level would reach the sink.
bool IsEnabled(Level level);

// Sends an already formatted message to the sink, if the level passes the filter.
void Write(Level level, std::string_view message);

template <class... Args>
void Log(Level level, std::format_string<Args...> fmt, Args&&... args) {
    if (!IsEnabled(level)) {
        return;
    }
    Write(level, std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace gl_engine::base
