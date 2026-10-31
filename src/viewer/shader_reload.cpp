#include "gl_engine/viewer/shader_reload.h"

#include <filesystem>

#include "gl_engine/base/log.h"

namespace gl_engine::viewer {

std::vector<base::WatchId> WatchShaders(base::FileWatcher& watcher, renderer::Renderer& renderer) {
    std::vector<base::WatchId> ids;
    for (const std::filesystem::path& path : renderer.ShaderFiles()) {
        ids.push_back(watcher.Watch(path, [&renderer](const std::filesystem::path& changed) {
            if (auto reloaded = renderer.ReloadShader(changed); !reloaded) {
                base::Log(base::Level::kError, "shader reload failed: {}",
                          base::ToString(reloaded.error()));
            }
        }));
    }
    return ids;
}

}  // namespace gl_engine::viewer
