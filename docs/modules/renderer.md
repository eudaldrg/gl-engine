---
aliases: [render, gl, opengl, shaders, shader reload, gl handle, test scene, debug draw]
sources: [src/renderer/**, tests/renderer/**, shaders/**]
decisions: [docs/adr/0002-errors-as-values-no-throw.md, docs/adr/0003-host-owns-window-and-context.md]
---

# renderer

Turns the scene and a `View` into pixels in a framebuffer the host provides; it owns no window,
context or loop (ADR 0003).

## Host contract

- `Renderer::Create(loader, {shader_dir})` needs a current GL 4.5 core context. `GlLoader` is
  `GlProc (*)(const char*)`, the signature of `glfwGetProcAddress` and of glad's loader, so a host
  passes its loader without a cast. Older or compatibility contexts return `ErrorCode::kUnsupported`;
  a shader that fails to build at start-up returns its compile log.
- `Render(target, view)` binds `target.framebuffer`, sets the viewport from the target and draws.
  It never picks a framebuffer on its own; the host decides, including framebuffer 0.
- Every GL object is a `GlHandle<Tag>` and must be destroyed while the context is current: the host
  destroys the `Renderer` before the context.
- The renderer loads GL through glad, whose function pointers are process-global. One context at a
  time is the M1 assumption.

## GL handles

`GlHandle<Tag>` is move-only and deletes its object on destruction. The tag supplies
`static void Delete(GLuint)`, which is what lets the unit tests check ownership with a counting tag
and no context. Factories (`CreateBuffer`, `CreateTexture(target)`, ...) use direct state access, so
objects exist before their first bind. A depth or integer attachment is a `Texture` with the right
storage format; no new handle type is needed for M2's offscreen targets.

## Shaders and reload

Programs are built from `<shader_dir>/<name>.vert` and `.frag`. `ShaderFiles()` lists every file for
the host to watch. `ReloadShader(path)` rebuilds every program using that file and swaps them in only
if all succeed, so a broken edit (code `kParse`, with the driver log) or a missing file (`kIo`) leaves
the previous programs drawing. Uniforms use explicit `layout(location = N)`, matched by constants in
`renderer.cpp`.

## Debug output

GL debug messages go to the log, mapped by severity. In debug builds a `GL_DEBUG_TYPE_ERROR` from the
API aborts: it means our code misused GL (ADR 0002). Messages from the shader compiler are logged at
debug level and never abort. Mesa reports compile failures there as errors, and those are
expected failures that `ReloadShader` already returns.

## Test scene

An icosphere (subdivision 3) and a cube resting on a ground grid on y = 0, either side of the x, y and
z axes in red, green and blue; Blinn-Phong under one directional light. Meshes are generated on the
CPU (`mesh.h`) and unit-tested for counts, unit normals and outward winding. The grid writes no depth:
the x and z axes lie exactly on grid lines, and two overlapping lines interpolate depth with
different rounding, so without that the axes come out dashed.

The golden images in `tests/render/golden/` depend on every constant here: camera, colours, layout,
light. Changing any of them means regenerating the goldens (`docs/tasks/render-tests.md`).
