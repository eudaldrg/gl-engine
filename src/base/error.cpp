#include "gl_engine/base/error.h"

#include <filesystem>
#include <format>

namespace gl_engine::base {

std::string_view ToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::kIo:
            return "io";
        case ErrorCode::kParse:
            return "parse";
        case ErrorCode::kGl:
            return "gl";
        case ErrorCode::kUnsupported:
            return "unsupported";
    }
    return "unknown";
}

std::string ToString(const Error& error) {
    const std::filesystem::path file{error.where.file_name()};
    return std::format("{}: {} ({}:{})", ToString(error.code), error.message,
                       file.filename().string(), error.where.line());
}

}  // namespace gl_engine::base
