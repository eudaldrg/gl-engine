# Roadmap

Goal: a small real-time renderer in C++23 and OpenGL 4.5 core that ends in a point-cloud viewer,
built as a public portfolio piece. See `context.md` for the why.

MVP = M1 + M2. Everything after M2 is optional by definition.

The renderer has two hosts from M1 on: the GLFW demo app and the offscreen render-test harness.
That keeps the renderer from owning the window, context or default framebuffer.

Platforms: Linux and Windows. Reference machines: navier, Linux, GeForce GT 1030 2 GB (low end);
a Windows machine with a Radeon RX 6600 XT (mid range). Numbers are reported for both, so they
cover two vendors and two tiers.

## M1: Base

- **Goal:** a window that renders a lit test object, with the tooling every later milestone uses.
- **Scope:**
  - renderer library with no windowing dependency; GLFW lives only in the demo app
  - the renderer never owns the window, context or main loop, and never assumes framebuffer 0
  - GL 4.5 core context, debug output (`glDebugMessageCallback`)
  - shader/program wrapper with hot reload
  - orbit and fly cameras, Dear ImGui panel, frame-time stats
  - debug draw: grid, axes, simple shapes, wireframe toggle
  - CMake presets, ASan/UBSan preset, clang-tidy, clang-format, unit tests
  - CI: build + unit tests on Linux and Windows; golden-image render tests on Linux (Mesa
    llvmpipe). The same render tests run locally on Windows, by hand.
- **Exit test:**
  - fresh clone builds with one preset command on Linux and on Windows
  - demo shows the test object; editing its shader on disk updates the image within 1 s, no restart
  - an invalid shader edit logs the compile error and keeps the last good program
  - CI green; the ASan/UBSan run is clean
- **Depends on:** nothing.
- **Design needed:**
  - GL loader choice (glad2 generated at configure time vs alternatives) and the no-vendoring rule
  - renderer / app boundary: what the host provides (context, proc-address function, target
    framebuffer, size, input events)
  - render-test harness: offscreen context, image comparison tolerance across llvmpipe and NVIDIA
- **Design status:** not started

## M2: Point-cloud viewer (MVP end)

- **Goal:** open a real 3D scan and inspect it: look around, color it, measure on it.
- **Scope:**
  - load PLY point clouds (positions, optional color/intensity)
  - render millions of points; point size control
  - color by height, by intensity, or by stored RGB
  - eye-dome lighting (EDL) so dense clouds read as shapes
  - minimal render-pass / framebuffer layer (needed by EDL and picking)
  - pick a point; measure distance between two picked points
  - frame-time numbers in the README
- **Exit test:**
  - a 10M-point cloud orbits at ≥ 60 fps at 1920×1080 on the GT 1030 (number provisional:
    confirm once M1 can measure); RX 6600 XT numbers reported alongside
  - two picked points on a cloud of known size give the expected distance within 0.1 %
  - loading a malformed PLY reports an error instead of crashing
- **Depends on:** M1.
- **Design needed:**
  - sample dataset: which scan, its license, how it is fetched (never committed)
  - GPU data layout for points; upload path for large files
  - picking: ID buffer vs depth readback
  - whether a synthetic generator is needed for deterministic tests
- **Design status:** not started

## M3: Mesh import

- **Goal:** load and view glTF 2.0 models with textures and lighting.
- **Scope:** glTF meshes via fastgltf or cgltf, textures (stb_image), Blinn-Phong lighting using
  glTF base color.
- **Exit test:** the Khronos glTF sample models from a chosen list load and render; numbers TBD.
- **Depends on:** M1.
- **Design needed:** glTF library; material mapping from glTF PBR to Blinn-Phong.
- **Design status:** not started

## Later / backlog

- PBR lighting (Cook-Torrance, image-based lighting)
- skeletal animation: glTF skins, GPU skinning
- fire and water effects (showcased in a separate repo; would be reimplemented, not ported)
- level of detail / octree for clouds beyond GPU memory; frustum culling with it
- LAS/LAZ LiDAR import; range images from laser profilers
- Qt desktop viewer (`QOpenGLWidget` host): Qt is a heavy install for anyone cloning; the M1
  boundary is already proven by two hosts, the GLFW demo and the offscreen render-test harness

## Non-goals

Vulkan/DX12, macOS (no test machine; GL capped at 4.1 there), ECS frameworks, scripting, an editor,
physics. Revisit after M2.
