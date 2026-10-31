// RGBA8 images for golden-image tests: PNG input and output, and comparison with a tolerance.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "gl_engine/base/error.h"

namespace gl_engine::render_tests {

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;  // rows top to bottom, four bytes per pixel
};

[[nodiscard]] base::Result<Image> LoadPng(const std::filesystem::path& path);
[[nodiscard]] base::Result<void> WritePng(const std::filesystem::path& path, const Image& image);

// A pixel differs when any channel is more than `channel` apart. The images match when at most
// `fraction` of the pixels differ.
struct Tolerance {
    int channel = 0;
    double fraction = 0.0;
};

struct Comparison {
    bool matches = false;
    bool same_size = false;
    std::size_t differing_pixels = 0;
    int max_channel_difference = 0;
    Image diff;  // differing pixels in red over a dimmed copy of `expected`; empty on size mismatch
};

Comparison Compare(const Image& actual, const Image& expected, const Tolerance& tolerance);

}  // namespace gl_engine::render_tests
