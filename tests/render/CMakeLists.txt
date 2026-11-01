# PNG input/output and image comparison, shared by the GL tests and unit-tested without a context.
add_project_library(render_test_image STATIC image.cpp stb.cpp)
target_include_directories(render_test_image PUBLIC .)
target_link_libraries(render_test_image PUBLIC gl_engine::base stb)

add_project_test(render_image_tests image_test.cpp)
target_link_libraries(render_image_tests PRIVATE render_test_image)

# Needs a display; on a headless machine run under xvfb-run. `ctest -L render` selects these.
add_project_test(render_tests render_test.cpp NO_GTEST_MAIN LABELS render)
target_link_libraries(render_tests PRIVATE render_test_image gl_engine::renderer glfw)
target_compile_definitions(render_tests PRIVATE
  GL_ENGINE_SHADER_DIR="${PROJECT_SOURCE_DIR}/shaders"
  GL_ENGINE_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
  GL_ENGINE_RENDER_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/output"
)
