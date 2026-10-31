add_project_executable(gl_demo main.cpp)
target_link_libraries(gl_demo PRIVATE gl_engine::base gl_engine::renderer gl_engine::viewer
                                      imgui_glfw)
link_glfw(gl_demo)
# Shaders are read from the source tree, so edits there reload in a running demo.
target_compile_definitions(gl_demo PRIVATE GL_ENGINE_SHADER_DIR="${PROJECT_SOURCE_DIR}/shaders")

if(BUILD_TESTING)
  # Needs a display; on a headless machine run under xvfb-run. `ctest -L demo` selects it.
  add_test(NAME demo_smoke COMMAND gl_demo --frames 3)
  set_tests_properties(demo_smoke PROPERTIES LABELS demo)
  # Build the demo with the test targets, so sanitizer runs (project_tests) include it.
  set_property(GLOBAL APPEND PROPERTY PROJECT_TEST_TARGETS gl_demo)
endif()
