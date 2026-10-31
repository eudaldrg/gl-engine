add_project_library(gl_engine_base STATIC
  error.cpp
  file_watcher.cpp
  log.cpp
)
add_library(gl_engine::base ALIAS gl_engine_base)
target_include_directories(gl_engine_base PUBLIC include)
