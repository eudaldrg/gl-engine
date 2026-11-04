// Hot reload: shader edits on disk rebuild the renderer's programs.
#pragma once

#include "base/file_watcher.h"
#include "renderer/renderer.h"

namespace gl_engine::viewer {

// Watches every file in renderer.ShaderFiles(). A change calls ReloadShader; a failure (compile
// error, deleted or unreadable file) is logged at error level and the previous program stays.
// The callbacks hold a reference to `renderer`: destroy the watcher before the renderer.
void WatchShaders(base::FileWatcher& watcher, renderer::Renderer& renderer);

}  // namespace gl_engine::viewer
