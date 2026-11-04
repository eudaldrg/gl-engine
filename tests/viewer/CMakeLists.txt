add_project_test(viewer_tests
  camera_test.cpp
  frame_stats_test.cpp
  viewer_test.cpp
)
target_link_libraries(viewer_tests PRIVATE gl_engine::viewer)
