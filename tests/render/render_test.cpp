// Golden-image tests: a hidden GLFW window provides the context, the renderer draws into the
// harness's own framebuffer, and the read-back image is compared with tests/render/golden/.
//
//   render_tests                    compare against the goldens
//   render_tests --update-golden    rewrite the goldens from this run (generate them on llvmpipe)

#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "gl_engine/base/log.h"
#include "gl_engine/renderer/gl_handle.h"
#include "gl_engine/renderer/renderer.h"
#include "image.h"

namespace gl_engine::render_tests {
namespace {

namespace fs = std::filesystem;
using renderer::Renderer;

constexpr int kSize = 256;

// Thresholds per device (docs/design/m1-base.md). Goldens come from llvmpipe, so llvmpipe must
// reproduce them almost exactly; GPUs rasterise and round differently.
constexpr Tolerance kLlvmpipeTolerance{.channel = 2, .fraction = 0.0};
constexpr Tolerance kHardwareTolerance{.channel = 8, .fraction = 0.005};

const fs::path kShaderDir{GL_ENGINE_SHADER_DIR};
const fs::path kGoldenDir{GL_ENGINE_GOLDEN_DIR};
const fs::path kOutputDir{GL_ENGINE_RENDER_OUTPUT_DIR};

bool& UpdateGolden() {
    static bool update = false;
    return update;
}

renderer::View FixedView() {
    renderer::View view;
    view.view = glm::lookAt(glm::vec3{2.5F, 3.5F, 6.5F}, glm::vec3{0.0F, 0.3F, 0.0F},
                            glm::vec3{0.0F, 1.0F, 0.0F});
    view.projection = glm::perspective(glm::radians(45.0F), 1.0F, 0.1F, 100.0F);
    return view;
}

class RenderTest : public ::testing::Test {
  protected:
    void SetUp() override {
        glfwSetErrorCallback([](int code, const char* description) {
            base::Log(base::Level::kError, "GLFW {}: {}", code, description);
        });
        ASSERT_EQ(glfwInit(), GLFW_TRUE) << "no display? On a headless machine run under xvfb-run";
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
        window_ = glfwCreateWindow(kSize, kSize, "render_tests", nullptr, nullptr);
        ASSERT_NE(window_, nullptr) << "could not create a GL 4.5 core context";
        glfwMakeContextCurrent(window_);
    }

    void TearDown() override {
        // Every GL object goes before the context.
        renderer_.reset();
        framebuffer_.Reset();
        color_.Reset();
        depth_.Reset();
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
    }

    // Creates the renderer and the harness's framebuffer: 256x256 RGBA8 colour, 24-bit depth,
    // no multisampling.
    void CreateRenderer(const fs::path& shader_dir) {
        auto created = Renderer::Create(glfwGetProcAddress, {.shader_dir = shader_dir});
        ASSERT_TRUE(created) << base::ToString(created.error());
        renderer_.emplace(std::move(*created));

        color_ = renderer::CreateTexture(GL_TEXTURE_2D);
        glTextureStorage2D(color_.Get(), 1, GL_RGBA8, kSize, kSize);
        depth_ = renderer::CreateTexture(GL_TEXTURE_2D);
        glTextureStorage2D(depth_.Get(), 1, GL_DEPTH_COMPONENT24, kSize, kSize);
        framebuffer_ = renderer::CreateFramebuffer();
        glNamedFramebufferTexture(framebuffer_.Get(), GL_COLOR_ATTACHMENT0, color_.Get(), 0);
        glNamedFramebufferTexture(framebuffer_.Get(), GL_DEPTH_ATTACHMENT, depth_.Get(), 0);
        ASSERT_EQ(glCheckNamedFramebufferStatus(framebuffer_.Get(), GL_FRAMEBUFFER),
                  static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE));
    }

    Image Render(const renderer::View& view) {
        renderer_->Render({.framebuffer = framebuffer_.Get(), .width = kSize, .height = kSize},
                          view);
        Image image{.width = kSize, .height = kSize, .rgba = {}};
        const std::size_t row = std::size_t{kSize} * 4;
        std::vector<std::uint8_t> bottom_up(row * kSize);
        glGetTextureImage(color_.Get(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                          static_cast<GLsizei>(bottom_up.size()), bottom_up.data());
        // GL's first row is the bottom one; PNG's is the top.
        image.rgba.reserve(bottom_up.size());
        for (std::size_t y = kSize; y-- > 0;) {
            const auto begin = bottom_up.begin() + static_cast<std::ptrdiff_t>(y * row);
            image.rgba.insert(image.rgba.end(), begin, begin + static_cast<std::ptrdiff_t>(row));
        }
        return image;
    }

    static void ExpectMatchesGolden(const std::string& name, const Image& actual) {
        const fs::path golden = kGoldenDir / (name + ".png");
        if (UpdateGolden()) {
            const auto written = WritePng(golden, actual);
            EXPECT_TRUE(written) << base::ToString(written.error());
            return;
        }
        const auto expected = LoadPng(golden);
        ASSERT_TRUE(expected) << base::ToString(expected.error())
                              << "; generate it with render_tests --update-golden";

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): GL returns text as GLubyte*.
        const std::string_view device{reinterpret_cast<const char*>(glGetString(GL_RENDERER))};
        const bool llvmpipe = device.find("llvmpipe") != std::string_view::npos;
        const Tolerance tolerance = llvmpipe ? kLlvmpipeTolerance : kHardwareTolerance;

        const Comparison result = Compare(actual, *expected, tolerance);
        if (result.matches) {
            return;
        }
        const fs::path actual_path = kOutputDir / (name + ".actual.png");
        const fs::path diff_path = kOutputDir / (name + ".diff.png");
        (void)WritePng(actual_path, actual);
        if (result.same_size) {
            (void)WritePng(diff_path, result.diff);
        }
        ADD_FAILURE() << name << " differs from its golden on " << device << ": "
                      << result.differing_pixels << " pixels over " << tolerance.channel
                      << "/255 (max difference " << result.max_channel_difference
                      << "). Actual: " << actual_path.string() << ", diff: " << diff_path.string();
    }

    GLFWwindow* window_ = nullptr;
    std::optional<Renderer> renderer_;
    renderer::Texture color_;
    renderer::Texture depth_;
    renderer::Framebuffer framebuffer_;
};

TEST_F(RenderTest, Scene) {
    ASSERT_NO_FATAL_FAILURE(CreateRenderer(kShaderDir));
    ExpectMatchesGolden("scene", Render(FixedView()));
}

TEST_F(RenderTest, WireframeWithoutGridOrAxes) {
    ASSERT_NO_FATAL_FAILURE(CreateRenderer(kShaderDir));
    renderer::View view = FixedView();
    view.wireframe = true;
    view.show_grid = false;
    view.show_axes = false;
    ExpectMatchesGolden("wireframe", Render(view));
}

// The exit test's reload path: a broken edit is reported and the last good program keeps
// drawing; fixing the file brings reload back.
TEST_F(RenderTest, BrokenShaderEditKeepsThePreviousProgram) {
    const fs::path dir = fs::temp_directory_path() / "gl_engine_render_reload";
    fs::remove_all(dir);
    fs::create_directories(dir);
    fs::copy(kShaderDir, dir);
    ASSERT_NO_FATAL_FAILURE(CreateRenderer(dir));
    const fs::path fragment = dir / "mesh.frag";

    std::ofstream{fragment} << "#version 450 core\nthis is not GLSL\n";
    const auto broken = renderer_->ReloadShader(fragment);
    ASSERT_FALSE(broken);
    EXPECT_EQ(broken.error().code, base::ErrorCode::kParse);
    EXPECT_NE(broken.error().message.find("mesh.frag"), std::string::npos);
    ExpectMatchesGolden("scene", Render(FixedView()));

    fs::remove(fragment);
    const auto missing = renderer_->ReloadShader(fragment);
    ASSERT_FALSE(missing);
    EXPECT_EQ(missing.error().code, base::ErrorCode::kIo);

    fs::copy_file(kShaderDir / "mesh.frag", fragment);
    EXPECT_TRUE(renderer_->ReloadShader(fragment));
    ExpectMatchesGolden("scene", Render(FixedView()));

    renderer_.reset();
    fs::remove_all(dir);
}

}  // namespace
}  // namespace gl_engine::render_tests

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    for (const std::string_view arg : std::span{argv, static_cast<std::size_t>(argc)}) {
        if (arg == "--update-golden") {
            gl_engine::render_tests::UpdateGolden() = true;
        }
    }
    return RUN_ALL_TESTS();
}
