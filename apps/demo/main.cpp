// GLFW host: owns the window, the GL context and the loop (ADR 0003), and translates GLFW events
// into viewer input events.
//
//   gl_demo              run until the window is closed or Escape is pressed
//   gl_demo --frames N   exit after N frames (used by the smoke test)

#include <glad/gl.h>
// glad must come first; GLFW_INCLUDE_NONE keeps GLFW from including a GL header of its own.
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>

#include <charconv>
#include <chrono>
#include <cstdio>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "gl_engine/base/error.h"
#include "gl_engine/base/file_watcher.h"
#include "gl_engine/base/log.h"
#include "gl_engine/renderer/renderer.h"
#include "gl_engine/viewer/gui.h"
#include "gl_engine/viewer/input.h"
#include "gl_engine/viewer/shader_reload.h"
#include "gl_engine/viewer/viewer.h"

namespace {

using gl_engine::viewer::InputEvent;
namespace base = gl_engine::base;
namespace viewer = gl_engine::viewer;

constexpr int kWidth = 1280;
constexpr int kHeight = 720;

struct Options {
    std::optional<long> frames;  // run forever when empty
};

std::optional<Options> ParseOptions(std::span<char*> args) {
    Options options;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg{args[i]};
        if (arg == "--frames" && i + 1 < args.size()) {
            const std::string_view value{args[++i]};
            long frames = 0;
            const auto [end, error] =
                std::from_chars(value.data(), value.data() + value.size(), frames);
            if (error != std::errc{} || end != value.data() + value.size() || frames < 0) {
                return std::nullopt;
            }
            options.frames = frames;
        } else {
            return std::nullopt;
        }
    }
    return options;
}

viewer::KeyCode ToKeyCode(int key) {
    switch (key) {
        case GLFW_KEY_W:
            return viewer::KeyCode::kW;
        case GLFW_KEY_A:
            return viewer::KeyCode::kA;
        case GLFW_KEY_S:
            return viewer::KeyCode::kS;
        case GLFW_KEY_D:
            return viewer::KeyCode::kD;
        case GLFW_KEY_Q:
            return viewer::KeyCode::kQ;
        case GLFW_KEY_E:
            return viewer::KeyCode::kE;
        case GLFW_KEY_TAB:
            return viewer::KeyCode::kTab;
        default:
            return viewer::KeyCode::kUnknown;
    }
}

std::optional<viewer::Button> ToButton(int button) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return viewer::Button::kLeft;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return viewer::Button::kRight;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return viewer::Button::kMiddle;
        default:
            return std::nullopt;
    }
}

// GLFW callbacks queue events here; the loop hands them to the viewer once per frame.
std::vector<InputEvent>& Events(GLFWwindow* window) {
    return *static_cast<std::vector<InputEvent>*>(glfwGetWindowUserPointer(window));
}

void InstallCallbacks(GLFWwindow* window) {
    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        Events(w).emplace_back(
            viewer::MouseMove{.position = {static_cast<float>(x), static_cast<float>(y)}});
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int /*mods*/) {
        if (const auto mapped = ToButton(button)) {
            Events(w).emplace_back(
                viewer::MouseButton{.button = *mapped, .pressed = action == GLFW_PRESS});
        }
    });
    glfwSetScrollCallback(window, [](GLFWwindow* w, double /*x*/, double y) {
        Events(w).emplace_back(viewer::Scroll{.delta = static_cast<float>(y)});
    });
    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int /*scancode*/, int action, int mods) {
        if (action == GLFW_REPEAT) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(w, GLFW_TRUE);
            return;
        }
        Events(w).emplace_back(viewer::Key{
            .key = ToKeyCode(key),
            .pressed = action == GLFW_PRESS,
            .mods = {.shift = (mods & GLFW_MOD_SHIFT) != 0,
                     .control = (mods & GLFW_MOD_CONTROL) != 0,
                     .alt = (mods & GLFW_MOD_ALT) != 0},
        });
    });
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        Events(w).emplace_back(viewer::Resize{.width = width, .height = height});
    });
}

// ImGui's GLFW backend, shut down before the Gui that it feeds.
class ImGuiGlfw {
  public:
    explicit ImGuiGlfw(GLFWwindow* window) {
        // Installs ImGui's callbacks, chained to the ones already set.
        ImGui_ImplGlfw_InitForOpenGL(window, true);
    }
    ~ImGuiGlfw() {
        ImGui_ImplGlfw_Shutdown();
    }
    ImGuiGlfw(const ImGuiGlfw&) = delete;
    ImGuiGlfw& operator=(const ImGuiGlfw&) = delete;
    ImGuiGlfw(ImGuiGlfw&&) = delete;
    ImGuiGlfw& operator=(ImGuiGlfw&&) = delete;
};

// Everything that holds GL objects lives in here, so it is all destroyed before the context.
// Locals are destroyed in reverse order: the watcher (whose callbacks reference the renderer),
// then the ImGui backends, then the renderer.
int Run(GLFWwindow* window, const Options& options) {
    auto created = gl_engine::renderer::Renderer::Create(glfwGetProcAddress,
                                                         {.shader_dir = GL_ENGINE_SHADER_DIR});
    if (!created) {
        base::Log(base::Level::kError, "{}", base::ToString(created.error()));
        return 1;
    }
    gl_engine::renderer::Renderer& renderer = *created;

    auto gui = viewer::Gui::Create();
    if (!gui) {
        base::Log(base::Level::kError, "{}", base::ToString(gui.error()));
        return 1;
    }
    const ImGuiGlfw imgui_glfw{window};

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    viewer::Viewer viewer{width, height};

    base::FileWatcher watcher;
    viewer::WatchShaders(watcher, renderer);

    std::vector<InputEvent>& events = Events(window);
    auto last = std::chrono::steady_clock::now();
    for (long frame = 0; !glfwWindowShouldClose(window); ++frame) {
        if (options.frames && frame >= *options.frames) {
            break;
        }
        glfwPollEvents();
        for (const InputEvent& event : events) {
            viewer.HandleEvent(event);
        }
        events.clear();
        watcher.Poll();

        const auto now = std::chrono::steady_clock::now();
        viewer.Update(now - last);
        last = now;

        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            // Minimised: nothing to draw, and no swap to wait on. Block until something happens,
            // waking often enough for the watcher.
            constexpr double kMinimisedWaitSeconds = 0.25;
            glfwWaitEventsTimeout(kMinimisedWaitSeconds);
            continue;
        }
        // The host owns the default framebuffer; the renderer draws wherever it is pointed.
        const gl_engine::renderer::RenderTarget target{
            .framebuffer = 0, .width = width, .height = height};
        renderer.Render(target, viewer.BuildView());

        ImGui_ImplGlfw_NewFrame();
        (*gui)->NewFrame();
        viewer.DrawPanel();
        (*gui)->Render(target);

        glfwSwapBuffers(window);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const auto options = ParseOptions(std::span{argv, static_cast<std::size_t>(argc)});
    if (!options) {
        std::println(stderr, "usage: gl_demo [--frames N]");
        return 2;
    }

    glfwSetErrorCallback([](int code, const char* description) {
        base::Log(base::Level::kError, "GLFW {}: {}", code, description);
    });
    if (glfwInit() != GLFW_TRUE) {
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);

    int result = 1;
    if (GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "gl-engine", nullptr, nullptr)) {
        std::vector<InputEvent> events;
        glfwSetWindowUserPointer(window, &events);
        InstallCallbacks(window);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        result = Run(window, *options);
        glfwDestroyWindow(window);
    } else {
        base::Log(base::Level::kError, "could not create a window with an OpenGL 4.5 core context");
    }
    glfwTerminate();
    return result;
}
