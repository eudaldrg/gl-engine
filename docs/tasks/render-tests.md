---
aliases: [render tests, golden images, goldens, llvmpipe, xvfb, update golden]
sources: [tests/render/**]
---

# Render tests

How to run the golden-image tests, read a failure, and regenerate the goldens.

## What runs

`render_tests` opens a hidden GLFW window for a GL 4.5 core context, renders into its own 256x256
framebuffer (RGBA8 plus 24-bit depth, no multisampling) from a fixed camera, reads the image back and
compares it with `tests/render/golden/<case>.png`. Cases: `scene`, `wireframe`, and a reload case that
breaks `mesh.frag` in a temporary copy of `shaders/` and checks that the image still matches `scene`.
`render_image_tests` tests the PNG and comparison code on its own, with no context.

The tolerance depends on `GL_RENDERER`. llvmpipe, which made the goldens, must match every channel
within 2/255. Any other device may have 0.5 % of pixels more than 8/255 apart. Both values are
provisional, until there are hardware runs to tune them on.

## Run

The tests need a display. On a desktop, `ctest` uses the GPU:

```bash
ctest --preset debug -L render --output-on-failure
```

Headless, or to reproduce CI exactly, use Xvfb and Mesa's software rasteriser:

```bash
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ctest --preset debug -L render --output-on-failure
```

Without a display every case fails at `glfwInit` with a message saying so; nothing is skipped
silently. The demo's smoke test (`-L demo`, `gl_demo --frames 3`) needs a display too. CI runs the
whole `ctest` and the sanitizer stage under `xvfb-run -a` with `LIBGL_ALWAYS_SOFTWARE=1`.

## A failure

The message names the device, how many pixels differ and the largest difference, and writes two
files to `build/debug/tests/render/output/`: `<case>.actual.png` and `<case>.diff.png`, where differing
pixels are red over a dimmed copy of the golden.

## Updating the goldens

After a change that is meant to alter the image, regenerate on llvmpipe, never on a GPU:

```bash
cmake --build --preset debug
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a build/debug/bin/render_tests --update-golden
```

Look at the new PNGs before committing; the PR diff shows them. Use `xvfb-run` even on a desktop:
with the NVIDIA driver installed, the real display may hand out NVIDIA's GL, while Xvfb always uses
Mesa. Without Linux, take the actual images from a CI run.

## Sanitizers

GLFW loads libX11 and libGL with `dlopen` and unloads them in `glfwTerminate`, before LeakSanitizer
scans. Memory those libraries keep in globals then reports as leaked, with `<unknown module>`
stacks. `link_glfw()` (`cmake/Dependencies.cmake`) links both libraries directly so they stay
mapped. That is not a suppression: real leaks still fail. If a new GLFW host shows those stacks,
link it with `link_glfw()`.
