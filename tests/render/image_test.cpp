#include "image.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>

namespace gl_engine::render_tests {
namespace {

Image Solid(int width, int height, std::uint8_t value) {
    Image image{.width = width, .height = height, .rgba = {}};
    image.rgba.assign(static_cast<std::size_t>(width * height * 4), value);
    return image;
}

TEST(CompareTest, IdenticalImagesMatch) {
    const Image image = Solid(4, 4, 100);
    const Comparison result = Compare(image, image, {});
    EXPECT_TRUE(result.matches);
    EXPECT_EQ(result.differing_pixels, 0U);
    EXPECT_EQ(result.max_channel_difference, 0);
}

TEST(CompareTest, DifferencesWithinTheChannelToleranceMatch) {
    const Comparison result = Compare(Solid(4, 4, 102), Solid(4, 4, 100), {.channel = 2});
    EXPECT_TRUE(result.matches);
    EXPECT_EQ(result.max_channel_difference, 2);
}

TEST(CompareTest, OnePixelOverTheChannelToleranceFailsWithoutAFraction) {
    Image actual = Solid(4, 4, 100);
    actual.rgba[5] = 110;  // pixel 1, green
    const Comparison result = Compare(actual, Solid(4, 4, 100), {.channel = 2});
    EXPECT_FALSE(result.matches);
    EXPECT_EQ(result.differing_pixels, 1U);
    EXPECT_EQ(result.max_channel_difference, 10);
    EXPECT_EQ(result.diff.rgba[4], 255);  // marked red in the diff
    EXPECT_EQ(result.diff.rgba[5], 0);
}

TEST(CompareTest, TheFractionAllowsSomeDifferingPixels) {
    Image actual = Solid(10, 10, 100);
    actual.rgba[0] = 0;  // 1 of 100 pixels
    EXPECT_TRUE(Compare(actual, Solid(10, 10, 100), {.channel = 8, .fraction = 0.01}).matches);
    EXPECT_FALSE(Compare(actual, Solid(10, 10, 100), {.channel = 8, .fraction = 0.005}).matches);
}

TEST(CompareTest, DifferentSizesNeverMatch) {
    const Comparison result =
        Compare(Solid(4, 4, 0), Solid(4, 5, 0), {.channel = 255, .fraction = 1});
    EXPECT_FALSE(result.matches);
    EXPECT_FALSE(result.same_size);
}

TEST(PngTest, WriteThenLoadRoundTrips) {
    Image image = Solid(3, 2, 0);
    for (std::size_t i = 0; i < image.rgba.size(); ++i) {
        image.rgba[i] = static_cast<std::uint8_t>(i * 10);
    }
    const auto path = std::filesystem::temp_directory_path() / "gl_engine_png_round_trip.png";
    ASSERT_TRUE(WritePng(path, image));
    const auto loaded = LoadPng(path);
    std::filesystem::remove(path);
    ASSERT_TRUE(loaded) << base::ToString(loaded.error());
    EXPECT_EQ(loaded->width, 3);
    EXPECT_EQ(loaded->height, 2);
    EXPECT_EQ(loaded->rgba, image.rgba);
}

TEST(PngTest, LoadingAMissingFileIsAnIoError) {
    const auto loaded = LoadPng("/nonexistent/gl_engine.png");
    ASSERT_FALSE(loaded);
    EXPECT_EQ(loaded.error().code, base::ErrorCode::kIo);
}

}  // namespace
}  // namespace gl_engine::render_tests
