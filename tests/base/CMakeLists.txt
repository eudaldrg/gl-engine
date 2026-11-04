add_project_test(base_tests
  error_test.cpp
  file_watcher_test.cpp
  log_test.cpp
)
target_link_libraries(base_tests PRIVATE gl_engine::base)
