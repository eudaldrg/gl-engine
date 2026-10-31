# 0003. The host owns the window and GL context; the renderer only draws

- Status: proposed
- Date: 2026-10-31
- Settled at: M1

## Context

The renderer should run under more than one host: the GLFW demo app, the offscreen render-test
harness, and possibly a Qt `QOpenGLWidget` later. These hosts disagree on who owns the main loop, who
creates the GL context, and which framebuffer is the target (Qt renders into its own FBO, never
framebuffer 0). If the renderer assumes any of these, adding a host means changing the renderer, and
the assumption spreads through every pass that binds a framebuffer or reads the window size.

## Options

### A. The host passes in what the renderer needs

- At start-up: a GL function loader (`void (*(*)(const char*))()`, a name to a function pointer), which every windowing library
  provides.
- Each frame: a render target, `{framebuffer id, width, height}`.
- The host creates the context, makes it current, and runs the loop. The renderer has no window
  type and reads no input.
- Cameras and input mapping live in a separate `viewer` library that takes host-neutral input
  events; each host translates its own events into them.

Adding a host means writing only the host. The renderer cannot open a window by itself, so even a
minimal demo needs a host.

### B. A `Window` interface in the renderer, GLFW as one implementation

Familiar, and a demo is a few lines. But the renderer drives the loop and owns the context, so the
test harness and a Qt host have to pretend to be windows, and the renderer still has to learn that
the target is not framebuffer 0.

## Decision

A, because it already has two hosts from M1 on (the GLFW demo and the offscreen test harness), and
neither has to bend to fit the renderer.

## Consequences

- No code in `renderer` binds framebuffer 0 or queries a window size; it uses the target it is given.
- Does not decide the shape of the per-frame API (one `render(target, scene, camera)` call or
  explicit passes); that is in the design doc.
- Does not decide threading: everything runs on the host's GL thread until a later decision says
  otherwise.
