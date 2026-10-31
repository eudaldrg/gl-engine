add_project_test(renderer_tests
  gl_handle_test.cpp
  mesh_test.cpp
)
target_link_libraries(renderer_tests PRIVATE gl_engine::renderer)
