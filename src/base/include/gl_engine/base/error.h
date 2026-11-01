// Errors as values (ADR 0002): one Error type, carried by Result<T> = std::expected<T, Error>.
#pragma once

#include <cstdint>
#include <expected>
#include <source_location>
#include <string>
#include <string_view>

namespace gl_engine::base {

enum class ErrorCode : std::uint8_t {
    kIo,           // a file is missing, unreadable or unwritable
    kParse,        // input was read but is malformed: a shader that fails to compile, ...
    kGl,           // the GL driver reported a failure
    kUnsupported,  // the platform lacks something required, e.g. GL 4.5 core
};

std::string_view ToString(ErrorCode code);

struct Error {
    ErrorCode code;
    std::string message;  // human-readable detail: shader log, file and line, ...
    // Where the Error was built. As a default member initialiser it records the aggregate
    // initialisation site, so `Error{ErrorCode::kIo, "..."}` captures the caller's line.
    std::source_location where = std::source_location::current();
};

// "io: <message> (file.cpp:42)"
std::string ToString(const Error& error);

template <class T>
using Result = std::expected<T, Error>;

}  // namespace gl_engine::base
