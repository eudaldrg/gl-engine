add_project_library(gl_engine_renderer STATIC
  gl_handle.cpp
  renderer.cpp
  shader.cpp
)
add_library(gl_engine::renderer ALIAS gl_engine_renderer)
target_include_directories(gl_engine_renderer PUBLIC include)
# GL types and glm appear in the public headers.
target_link_libraries(gl_engine_renderer PUBLIC gl_engine::base glad_gl_core_45 glm::glm)
