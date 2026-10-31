# gl-engine

A small real-time 3D renderer in modern C++ (C++23, OpenGL 4.5 core), built stage by stage, ending
in a point-cloud viewer.

The idea comes from an engine assignment in a game design master's. The goal here is a modern C++
take on that kind of engine: RAII ownership, explicit dependencies instead of singletons, and
errors returned as values.

Status: M1 (base) implemented: a GLFW demo renders a lit test scene with debug draw, orbit and fly
cameras, an ImGui panel and shader hot reload. `docs/roadmap.md` has what comes next.

## Requirements

- CMake 3.28+, Ninja, and a C++23 compiler: GCC 14, Clang 18 or Visual Studio 2022 17.8, or newer.
- Python 3 with `jinja2` (`pip install jinja2`): the OpenGL loader is generated at configure time.
- A GPU and driver with OpenGL 4.5 core.
- Linux only: the X11 and GL development packages. On Debian or Ubuntu:

  ```bash
  sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
      libgl-dev xvfb mesa-utils
  ```

  `xvfb` and `mesa-utils` are for running the render tests without a display.

Every other dependency (glm, glad2, GLFW, Dear ImGui, stb, GoogleTest) is downloaded by CMake at a
pinned version on the first configure.

## Build and run

Linux:

```bash
cmake --preset debug
cmake --build --preset debug
./build/debug/bin/gl_demo
```

Windows, from a Developer PowerShell for Visual Studio:

```powershell
cmake --preset debug
cmake --build --preset debug
.\build\debug\bin\gl_demo.exe
```

The demo reads its shaders from `shaders/` in the source tree: edit one while it runs and the image
updates within a second. A broken edit logs the compile error and the last good shader keeps drawing.
Left drag orbits, right drag pans, scroll zooms, Tab switches to a fly camera (WASD, Q/E), Escape
quits.

Tests: `ctest --preset debug`. The render and demo tests need a display; `docs/tasks/testing.md` and
`docs/tasks/render-tests.md` cover running them headless and updating the golden images.

### Without Python or jinja2

Generate the loader once on the [glad2 web generator](https://gen.glad.sh/): API `gl` version 4.5,
profile `core`, language C. Unpack the archive into `third_party/glad-generated/`, so that
`third_party/glad-generated/src/gl.c` and `third_party/glad-generated/include/glad/gl.h` exist. The
folder is git-ignored, and when present, configure uses it instead of generating the loader.

## Docs

`docs/architecture.md` describes the components and their contracts, `docs/adr/` the decisions,
`docs/modules/` each library, and `docs/tasks/` each procedure.
