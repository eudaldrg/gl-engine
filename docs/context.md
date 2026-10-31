# Engine project: context for a Claude session on navier

## What this is

A small real-time 3D engine/renderer in modern C++, started from scratch as a public portfolio piece on GitHub (`eudaldrg`).

**How the work is split.** Eudald is the architect and reviewer; Claude does the implementing. This is how he worked for the last year at his previous company.
- **Eudald:** owns the design. Design docs and ADRs, the module boundaries, the plan, and the breakdown into tickets.
- **Claude:** implements each ticket on its own branch, as a focused PR with tests and a clear description.
- **Eudald:** reviews every PR before merging, asks for changes, and hand-adjusts small parts himself.
- **Claude should:** push back on designs it thinks are wrong, keep PRs small enough to review properly, and never merge its own work.

That process is part of the showcase too. The repo should visibly show design → tickets → reviewed PRs: ADRs in `docs/adr/`, GitHub issues, and PR history. It's real evidence of the AI-assisted engineering workflow that Cognex is pushing.

## Why (portfolio angle)

- Eudald has 9 years of C++ in HFT: performance, Qt visualization tools, build speed. He's doing a UOC master's in game design (2025–).
- The target he has in mind is **Cognex Girona**: 3D smart cameras and laser profilers. That team builds cross-platform C++ 3D visualization for point clouds and range images on Linux (C++20, NixOS, Valgrind), and has a legacy OpenGL 1/2 + wxWidgets stack to modernise.
- A clean, cross-platform, modern-GL engine with a point-cloud viewer mode maps directly to that work.
- It also keeps his graphics skills fresh in general.

## Prior work it builds on (Windows PC, UOC coursework; private, not on navier)

- **Graphics engines course:** DirectX 11 engine on a UOC framework. FBX SDK import, skeletal animation (cal3d), PhysX, Lua hot reload, AntTweakBar debug UI. Old-school code; **don't port it**. Reimplement the ideas.
- **VFX PAC2 → `opengl-fire-effect` (being published on GitHub):**
  - OpenGL 3.3 / GLSL, SDL2
  - ping-pong FBO fire simulation, 1D colour ramp
  - DuDv heat shimmer, bloom on the fire quad only
  - water reflection/refraction with clip planes, depth fade
  - point light with flicker
- **VFX PAC1:** software-rendered demoscene effects (plasma, tunnel, rotozoom, CPU fire).
- **Programació Gràfica:** raylib 2D games, bloom post-process, particles.

## Suggested stack (open to discussion)

- **Language:** C++23 (ADR 0001).
- **Build:** CMake ≥ 3.25 with `CMakePresets.json`, Ninja. Clang/GCC on Ubuntu, MSVC on Windows.
- **Windowing/GL:** GLFW + glad2, **OpenGL 4.5 core with DSA**. Vulkan is a non-goal for now.
- **Math:** glm. **Debug UI:** Dear ImGui, replacing AntTweakBar. **Images:** stb_image.
- **Meshes:** glTF 2.0 via fastgltf or cgltf, **not FBX**: the FBX SDK is proprietary and painful on Linux.
- **Dependencies:** FetchContent, pinned versions. Nothing vendored in the repo.
- **Quality:**
  - GitHub Actions matrix (ubuntu-latest + windows-latest)
  - ASan/UBSan preset, clang-tidy, clang-format
  - doctest/Catch2 for maths, loaders and the camera
  - RenderDoc for GPU debugging

## Stages (each one finished, documented and shippable)

1. **Base:**
   - window, GL context, debug output (`glDebugMessageCallback`)
   - shader/program wrapper with hot reload
   - orbit + fly cameras, ImGui panel
   - CI green on both OSes
2. **Mesh import:** glTF meshes + PBR-ish materials, textures, a simple Blinn-Phong or basic PBR pass, frame timing stats.
3. **Effects:** port the fire + water from `opengl-fire-effect` into the engine's structure, with a render-pass/FBO abstraction.
4. **Skeletal animation:** glTF skins, GPU skinning.
5. **Point-cloud / range-image viewer** (the Cognex-shaped piece):
   - load PLY/PCD or a synthetic laser-profile scan
   - millions of points, colour by height/intensity
   - point sprites or splats, orbit camera, picking/measurement
   - frame-time numbers in the README
6. **Optional:** embed the renderer in a Qt `QOpenGLWidget`. That's a desktop viewer app, which Eudald knows well from his Qt tooling work.

Each stage starts with a design doc and a ticket breakdown. Its README section gets a GIF/screenshot and a short "design decisions" section, in the same style as the ADRs in `crypto_trader`.

## Non-goals (for now)

Vulkan/DX12, ECS frameworks, scripting, an editor, physics. Revisit these after stage 5.

## Timing

Not urgent. Having stages 1–2 public before the Cognex conversations deepen (Nov 2026) would be nice, not required. Quality over speed.
