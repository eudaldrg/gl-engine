---
aliases: [camera, cameras, orbit camera, fly camera, input, input events, imgui, panel, frame stats]
sources: [src/viewer/**, tests/viewer/**]
decisions: [docs/adr/0003-host-owns-window-and-context.md]
---

# viewer

Host-neutral interactive behaviour: input events, orbit and fly cameras, frame stats, the ImGui panel,
and the wiring from file changes to shader reload.

## Input events

Hosts translate their own events into `InputEvent` (`MouseMove`, `MouseButton`, `Scroll`, `Key`,
`Resize`); the viewer never includes GLFW or Qt. `KeyCode` lists only the keys the viewer reacts to;
hosts map everything else to `kUnknown`. `Resize` carries framebuffer pixels; a zero size (minimised
window) keeps the previous aspect.

## Controls

| Input | Orbit camera | Fly camera |
|---|---|---|
| left drag | rotate around the target | look |
| right or middle drag | pan the target | look |
| scroll | zoom (scales the distance) | change speed |
| W A S D, Q E | | move; Q and E along world up; Shift is faster |
| Tab | switch to fly, from the orbit pose | switch back to orbit |

Both cameras share one angle convention (yaw about +y, pitch from the horizon, zero looks down -z), so
switching copies the angles. Fly movement scales with the frame time from `std::chrono::steady_clock`,
with no clamping (architecture: C++ conventions). Mouse and keyboard presses are ignored while ImGui
wants them; releases always go through, so a drag that ends over the panel still ends.

## Gui

`Gui::Create()` makes the ImGui context with no `.ini` file and starts the OpenGL3 backend. The host
owns the platform side: it starts its backend (for GLFW, `ImGui_ImplGlfw_InitForOpenGL`) after
`Create` and shuts it down before destroying the `Gui`. Per frame: the platform backend's `NewFrame`,
then `Gui::NewFrame`, `Viewer::DrawPanel`, `Gui::Render(target)`. The OpenGL3 backend loads GL
functions with its own loader, independent of glad.

## Shader reload

`WatchShaders(watcher, renderer)` registers every file in `Renderer::ShaderFiles()`. A change calls
`ReloadShader` and logs a failure at error level. The callbacks hold a reference to the renderer:
destroy the watcher first. In the demo the watcher is declared after the renderer for that reason.
