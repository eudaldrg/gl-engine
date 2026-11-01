add_project_library(gl_engine_viewer STATIC
  camera.cpp
  frame_stats.cpp
  gui.cpp
  shader_reload.cpp
  viewer.cpp
)
add_library(gl_engine::viewer ALIAS gl_engine_viewer)
target_include_directories(gl_engine_viewer PUBLIC include)
target_link_libraries(gl_engine_viewer
  PUBLIC gl_engine::base gl_engine::renderer glm::glm
  PRIVATE imgui_core
)
