#include "image.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstdlib>
#include <format>
#include <memory>

namespace gl_engine::render_tests {

namespace {

constexpr int kChannels = 4;
constexpr std::uint8_t kOpaque = 255;

struct StbiFree {
    void operator()(stbi_uc* pixels) const {
        stbi_image_free(pixels);
    }
};

std::size_t ByteCount(int width, int height) {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
}

}  // namespace

base::Result<Image> LoadPng(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;
    int file_channels = 0;
    const std::unique_ptr<stbi_uc, StbiFree> pixels{
        stbi_load(path.string().c_str(), &width, &height, &file_channels, kChannels)};
    if (!pixels) {
        return std::unexpected{
            base::Error{base::ErrorCode::kIo,
                        std::format("cannot read {}: {}", path.string(), stbi_failure_reason())}};
    }
    Image image{.width = width, .height = height, .rgba = {}};
    image.rgba.assign(pixels.get(), pixels.get() + ByteCount(width, height));
    return image;
}

base::Result<void> WritePng(const std::filesystem::path& path, const Image& image) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    const int stride = image.width * kChannels;
    if (stbi_write_png(path.string().c_str(), image.width, image.height, kChannels,
                       image.rgba.data(), stride) == 0) {
        return std::unexpected{
            base::Error{base::ErrorCode::kIo, std::format("cannot write {}", path.string())}};
    }
    return {};
}

Comparison Compare(const Image& actual, const Image& expected, const Tolerance& tolerance) {
    Comparison result;
    result.same_size = actual.width == expected.width && actual.height == expected.height &&
                       actual.rgba.size() == expected.rgba.size();
    if (!result.same_size) {
        return result;
    }
    result.diff = expected;
    for (std::size_t pixel = 0; pixel < expected.rgba.size(); pixel += kChannels) {
        int pixel_difference = 0;
        for (std::size_t channel = 0; channel < kChannels; ++channel) {
            const int difference =
                std::abs(int{actual.rgba[pixel + channel]} - int{expected.rgba[pixel + channel]});
            pixel_difference = std::max(pixel_difference, difference);
        }
        result.max_channel_difference = std::max(result.max_channel_difference, pixel_difference);
        const bool differs = pixel_difference > tolerance.channel;
        if (differs) {
            ++result.differing_pixels;
        }
        for (std::size_t channel = 0; channel < 3; ++channel) {
            std::uint8_t& value = result.diff.rgba[pixel + channel];
            const std::uint8_t marker = channel == 0 ? kOpaque : std::uint8_t{0};
            value = differs ? marker : static_cast<std::uint8_t>(value / 4);
        }
        result.diff.rgba[pixel + 3] = kOpaque;
    }
    const auto pixel_count = static_cast<double>(expected.rgba.size() / kChannels);
    result.matches =
        static_cast<double>(result.differing_pixels) <= tolerance.fraction * pixel_count;
    return result;
}

}  // namespace gl_engine::render_tests
