# M1: Base

- Design status: in progress

## Goal and exit test

A window that renders a lit test object, with the tooling every later milestone uses.

- A fresh clone builds with one preset command on Linux and on Windows.
- The demo shows the test object; editing its shader on disk updates the image within 1 s, with no
  restart.
- An invalid shader edit logs the compile error and keeps the last good program.
- CI is green; the ASan/UBSan run is clean.

## Scope

- `renderer`: GL 4.5 core start-up check, debug output, RAII GL handles, shader programs with hot
  reload, a lit test object, debug draw (grid, axes, simple shapes, wireframe toggle).
- `viewer`: orbit and fly cameras, host-neutral input events, ImGui panel, frame-time stats.
- `apps/demo`: GLFW window, context and loop.
- `tests/render`: offscreen host and golden-image comparison.
- Build: CMake presets, Ninja, FetchContent dependencies, GL loader, sanitizer preset,
  clang-format and clang-tidy, unit tests, CI on Linux and Windows. Hooks, CI and branch protection
  come from `core:setup-cpp-repo`.

## Non-goals

No file loading (that is M2's `assets`), no render-pass layer beyond what debug draw needs, no
threading.

## Components touched

All new: `renderer`, `viewer`, `apps/demo`, `tests/render` (see `../architecture.md`).

## Dependencies

All through FetchContent at pinned tags.

| Library | Used by | Note |
|---|---|---|
| glad2 | `renderer` | GL 4.5 core loader, generated at configure time (needs Python and jinja2); a pre-generated folder is the fallback |
| glm | `renderer`, `viewer` | math |
| Dear ImGui | `viewer`, `apps/demo` | OpenGL backend in `viewer`, GLFW backend in the demo only |
| GLFW | `apps/demo` | the only windowing dependency |
| GoogleTest | tests | |
| stb_image_write | `tests/render` | writes PNG for golden images |

## Interfaces and data

To be filled as the open questions below are settled.

## Flows

To be filled: start-up (context check), frame, shader reload (success and compile failure), resize,
render test (pass, fail with diff image).

## Decisions

- ADR 0001: C++23, GCC 14 / Clang 18 / MSVC 17.8.
- ADR 0002: `std::expected` for handleable failures, abort on broken invariants, no throwing.
- ADR 0003: the host owns the window, context and loop.
- Per-frame API (register row 6): retained scene plus one call per frame,
  `render(target, view)`. Passes are internal to `renderer`; queries such as picking are separate
  calls.
- Design doc choices: glad2 at configure time, FetchContent, GoogleTest, glm (architecture register
  rows 4, 5, 11, 12).

## Assumptions about later milestones

- M2's EDL and picking need offscreen targets with depth; the RAII handles and render target type
  must allow FBOs with colour, depth and integer attachments without changing their API.

## Open questions

- **blocking** RAII handle design for GL objects (row 10).
- **blocking** Shader hot reload: polling or OS file watch (row 7).
- **blocking** Render-test harness: how the offscreen context is created on CI, and the image
  tolerance across llvmpipe, NVIDIA and AMD (row 8).
- **blocking** What `Error` contains (row 9).
- **non-blocking** Logging (row 13).
- **non-blocking** What the lit test object is (a procedural sphere or cube needs no file loading).
