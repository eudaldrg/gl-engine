# Architecture

How the engine is split, how data moves through it, and which decisions shape it. The roadmap
(`roadmap.md`) says what gets built when; ADRs (`adr/`) hold the decisions that are expensive to
reverse.

## Components

| Component | Responsibility | From |
|---|---|---|
| `renderer` | Turn GPU-resident scene data and a camera into pixels in a target framebuffer. | M1 |
| `assets` | Parse files on disk into plain CPU data (PLY, later glTF and images). No GL. | M2 |
| `viewer` | Interactive behaviour, host-neutral: cameras, input mapping, ImGui panels, frame stats. | M1 |
| `apps/demo` | GLFW host: window, context, main loop, GLFW events translated to `viewer` events. | M1 |
| `tests/render` | Offscreen host: renders into an FBO and compares against golden images. | M1 |

`renderer` and `viewer` are libraries; the two hosts are executables. `assets` has no GL dependency,
so its parsers are tested without a context.

## Data flow

```
 file on disk ──► assets ──► CPU data (PointCloud, later Mesh) ──► renderer: upload to GPU buffers
                                                                        │
 host events ──► viewer (camera, panels) ──► frame params ──────────────┤
                                                                        ▼
 host ──► RenderTarget {fbo, width, height} ──────────────────► renderer: draw into the target
                                                                        │
                                       picking readback ◄───────────────┘ ──► viewer (distance)
```

Two rules hold the shape:

1. **The host owns the window, the context and the loop** (ADR 0003). `renderer` never binds
   framebuffer 0, reads input or queries a window.
2. **Only `assets` parses data files.** `renderer` receives CPU data and never sees a file, except
   its own shader sources, which it reads for hot reload.

Everything runs on the host's GL thread until a decision says otherwise (register row 19).

## Contracts

These are what other components depend on, and so the expensive things to change.

- **Host to renderer.** Created once with the host's GL function loader
  (`void* (*)(const char*)`); fails with an `Error` if the context is not GL 4.5 core. Each frame it
  takes a `RenderTarget {framebuffer id, width, height}` plus frame parameters (camera matrices,
  display settings). The exact per-frame API is open (row 6).
- **Host to viewer.** Host-neutral input events: mouse move, mouse button, scroll, key, resize.
  Each host maps its own events onto these; the viewer never includes GLFW or Qt.
- **assets to renderer.** Plain structs of contiguous arrays, passed as `std::span`, structure of
  arrays for point clouds (positions; optional colour and intensity). No ownership crosses: the
  renderer copies into GPU buffers.
- **Errors.** Fallible calls return `std::expected<T, Error>`; broken invariants abort (ADR 0002).
- **Golden images.** PNG files committed under `tests/render/golden/`, one per test case, compared
  with a tolerance (row 8).

## Storage

Nothing persists at run time. ImGui's `.ini` layout file is turned off. Sample datasets are downloaded
to a git-ignored cache directory by a fetch script, pinned by URL and SHA-256 (row 16). Golden images
are the only test data in the repository.

## Trust boundaries

The only untrusted input is data files: PLY now, glTF and images later. Every parser in `assets`
bounds-checks against the file size and reports malformed input as an `Error` (M2 exit test). The
PLY parser is fuzzed. The fetch script verifies checksums before anything is read. There are no
credentials, network services or user accounts.

## C++ conventions

Mechanical rules live in `.clang-format` and `.clang-tidy`, generated from the shared base rules
(`core:sync-lint-rules`), not in prose. The patterns below are the ones a linter cannot enforce:

- GL objects are owned by move-only RAII handles; there are no `Init`/`Destroy` pairs.
- No singletons or global mutable state. The host owns the top-level objects and passes references
  down; if parameter lists grow, they are grouped in a context struct, not hidden in a global.
- Objects are held by value, or by `std::unique_ptr` when identity or polymorphism needs it. Raw
  pointers never own.
- Polymorphism only where there is more than one implementation.
- Units and constants are `constexpr`; no macros except for platform detection.
- Paths are `std::filesystem::path`; time is `std::chrono::steady_clock`, with no clamping of frame
  deltas.
- Functions returning `std::expected` are `[[nodiscard]]`.

## Out of scope

Vulkan or DX12, macOS, an ECS, scripting, physics, an editor, networking. See `roadmap.md`
non-goals.

## Decision register

| # | Question | Status | Settled by | Depends on | ADR |
|---|---|---|---|---|---|
| 1 | C++ standard and compiler floor | proposed | M1 | | 0001 |
| 2 | Error handling policy | proposed | M1 | 1 | 0002 |
| 3 | Renderer / host boundary | proposed | M1 | | 0003 |
| 4 | GL loader: glad2 generated at configure time; pre-generated folder as fallback | design doc | M1 | 3 | |
| 5 | Unit-test framework: GoogleTest | design doc | M1 | | |
| 6 | Per-frame renderer API: retained scene, one `render(target, view)` call | design doc | M1 | 3 | |
| 7 | Shader hot reload: poll file times or OS file watch | open | M1 | | |
| 8 | Render-test harness: offscreen context, image tolerance across llvmpipe/NVIDIA/AMD | open | M1 | 3 | |
| 9 | What `Error` contains | design doc | M1 | 2 | |
| 10 | GL object ownership: RAII handle design | open | M1 | 2 | |
| 11 | Dependency fetching: FetchContent at pinned tags | design doc | M1 | | |
| 12 | Math library: glm | design doc | M1 | | |
| 13 | Logging | open | M1 | 2 | |
| 14 | PLY parser: write one or use a library | open | M2 | 2 | |
| 15 | GPU layout and upload path for large point clouds | open | M2 | 6 | |
| 16 | Sample dataset, its licence, and how it is fetched | open | M2 | | |
| 17 | Picking: ID buffer or depth readback | open | M2 | 6 | |
| 18 | Render-pass / framebuffer layer shape | open | M2 | 6 | |
| 19 | Background loading thread for large files | open | M2 | 3, 14 | |
| 20 | glTF library: fastgltf or cgltf | open | M3 | 14 | |
