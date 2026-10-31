# M1: Base

- Design status: ready

## Goal and exit test

A window that renders a lit test scene, with the tooling every later milestone uses.

- A fresh clone builds with one preset command on Linux and on Windows.
- The demo shows the test scene; editing a shader on disk updates the image within 1 s, with no
  restart.
- An invalid shader edit logs the compile error and keeps the last good program.
- CI is green; the ASan/UBSan run is clean.

## Scope

- `base`: `Error`, logging, `FileWatcher`.
- `renderer`: GL 4.5 core start-up check, debug output routed to the log, RAII GL handles, shader
  programs with reload, the test scene, debug draw (grid, axes, simple shapes, wireframe toggle).
- `viewer`: orbit and fly cameras, host-neutral input events, ImGui panel, frame-time stats, wiring
  the `FileWatcher` to shader reload.
- `apps/demo`: GLFW window, context and loop.
- `tests/render`: offscreen host and golden-image comparison.
- Build: CMake presets, Ninja, FetchContent dependencies, GL loader, sanitizer presets,
  clang-format and clang-tidy, unit tests, CI on Linux and Windows. Hooks, CI and branch protection
  come from `core:setup-cpp-repo`.

ImGui's `.ini` layout file is turned off.

**Test scene:** three axes (x red, y green, z blue), a procedural icosphere and a cube, lit by one
directional light with Blinn-Phong.

## Non-goals

No file loading (that is M2's `assets`), no render-pass layer beyond what debug draw needs, no
threading, no scripting (but `FileWatcher` must not prevent it later).

## Components touched

All new: `base`, `renderer`, `viewer`, `apps/demo`, `tests/render` (see `../architecture.md`).

## Dependencies

All through FetchContent at pinned tags.

| Library | Used by | Note |
|---|---|---|
| glad2 | `renderer` | GL 4.5 core loader, generated at configure time (needs Python and jinja2); a pre-generated folder is the fallback |
| glm | `renderer`, `viewer` | math |
| Dear ImGui | `viewer`, `apps/demo` | OpenGL backend in `viewer`, GLFW backend in the demo only |
| GLFW | `apps/demo`, `tests/render` | the only windowing dependency |
| GoogleTest | tests | |
| stb_image_write | `tests/render` | writes PNG for golden images |

## Interfaces and data

Shapes, not final spelling. Namespaces follow the library names.

### `base`: errors

```cpp
enum class ErrorCode { io, parse, gl, unsupported };

struct Error {
    ErrorCode code;
    std::string message;              // human-readable detail: shader log, file and line, ...
    std::source_location where = std::source_location::current();
};

template <class T> using Result = std::expected<T, Error>;
```

One type everywhere. If a module later needs richer errors, it defines its own type (say
`ShaderError` with per-line diagnostics) that converts to `Error`, and returns that type only on its
internal API. Not inheritance: `std::expected` stores its error by value, so a derived error would be
sliced. The migration is local to that module.

### `base`: logging

```cpp
enum class Level { trace, debug, info, warn, error };

void set_level(Level);                                 // runtime filter; below it is dropped
void set_sink(std::function<void(Level, std::string_view)>);  // default: stderr
template <class... A> void log(Level, std::format_string<A...>, A&&...);
```

Later addition: a compile-time floor (CMake option) under which calls compile to nothing.
Arguments are still evaluated at the call site unless that addition uses a macro; decide then.

### `base`: file watching

```cpp
class FileWatcher {
public:
    using Callback = std::function<void(const std::filesystem::path&)>;
    WatchId watch(std::filesystem::path, Callback);
    void unwatch(WatchId);
    void poll();          // host calls once per frame; checks times at most every 250 ms
};
```

The API does not expose polling as a mechanism: callers only register paths. Swapping in OS
notifications later changes the implementation, not the callers. A later Lua binding registers its
scripts on the same watcher.

### `renderer`: GL handles

```cpp
template <class Tag> class GlHandle {     // move-only, deletes on destruction
public:
    GLuint get() const;
};
using Buffer = GlHandle<BufferTag>;  // also Texture, VertexArray, Framebuffer, Shader, Program

Buffer create_buffer();   // glCreateBuffers; one factory per type
```

Every handle must be destroyed while its context is current: the host destroys the `Renderer`
before the context.

### `renderer`: host contract

```cpp
using GlProc = void (*)();
using GlLoader = GlProc (*)(const char*);

struct RenderTarget { GLuint framebuffer; int width; int height; };

struct View {
    glm::mat4 view, projection;
    glm::vec3 light_direction;
    bool wireframe; bool show_grid; bool show_axes;
};

class Renderer {
public:
    static Result<Renderer> create(GlLoader, RendererConfig);   // fails unless GL >= 4.5 core
    void render(const RenderTarget&, const View&);
    std::span<const std::filesystem::path> shader_files() const;
    Result<void> reload_shader(const std::filesystem::path&);    // keeps the old program on error
};
```

`RendererConfig` holds the shader directory: the source tree in dev builds (CMake passes it in), a
directory next to the binary otherwise.

### `viewer`: input events

```cpp
struct MouseMove { glm::vec2 position; };
struct MouseButton { Button button; bool pressed; };
struct Scroll { float delta; };
struct Key { KeyCode key; bool pressed; Modifiers mods; };
struct Resize { int width; int height; };
using InputEvent = std::variant<MouseMove, MouseButton, Scroll, Key, Resize>;
```

## Flows

- **Start-up:** host creates the context, calls `Renderer::create`. Wrong GL version or missing
  extensions: `Error` with code `unsupported`, the host prints it and exits non-zero.
- **Frame:** host forwards events to `viewer`, calls `FileWatcher::poll`, `viewer` updates the
  camera and builds `View`, `Renderer::render(target, view)`, ImGui draws on top, host swaps.
- **Shader reload, success:** watcher reports a path, `viewer` calls `reload_shader`, the new program
  is used from the next frame.
- **Shader reload, compile error:** `reload_shader` returns `Error` with the compile log, the log
  gets it at `error`, the old program stays.
- **Shader file deleted or unreadable:** same as a compile error, code `io`.
- **Resize:** host passes the new size in `RenderTarget`; the projection is updated from `Resize`.
- **GL debug message:** the debug callback logs it; in debug builds a GL error is a broken invariant
  and aborts (ADR 0002).

## Decisions

- ADR 0001: C++23, GCC 14 / Clang 18 / MSVC 17.8.
- ADR 0002: `std::expected` for handleable failures, abort on broken invariants, no throwing.
- ADR 0003: the host owns the window, context and loop.
- Design doc choices, architecture register: glad2 (row 4), GoogleTest (5), retained scene with one
  `render` call (6), polled `FileWatcher` (7), render-test harness (8), single `Error` type (9), `GlHandle<Tag>` (10),
  FetchContent (11), glm (12), own logger (13).
- Sanitizers: the `asan-ubsan` preset on Linux and in CI; an `asan` preset on Windows
  (`/fsanitize=address`, MSVC has no UBSan), run by hand.

## Assumptions about later milestones

- M2's EDL and picking need offscreen targets with depth; `GlHandle` and `RenderTarget` must allow
  FBOs with colour, depth and integer attachments without changing their API.
- Scripting, if it comes, binds to `FileWatcher` and to `viewer`, never to `renderer` internals.

## Render tests

- **Context:** the harness is a GLFW host with a hidden window, rendering into its own FBO, never
  framebuffer 0. On Linux CI it runs under `xvfb-run` with Mesa llvmpipe; locally it uses the GPU.
- **Cases:** fixed scene, fixed camera, 256×256, no MSAA. One golden PNG per case under
  `tests/render/golden/`, generated on llvmpipe.
- **Comparison:** thresholds are harness parameters. Defaults (provisional, retuned after the first
  hardware runs): on llvmpipe, every channel within 2/255; on hardware, at most 0.5 % of pixels
  differ by more than 8/255.
- **On failure:** the actual image and a diff image are written to the build directory.
- **Updating:** `--update-golden` rewrites the goldens; the PR diff shows the image changes.

## Open questions

None blocking.
