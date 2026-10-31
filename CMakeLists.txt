cmake_minimum_required(VERSION 3.28)
project(gl_engine LANGUAGES CXX)

# Compiler floor, warnings, sanitizers, GoogleTest and the add_project_* helpers.
include(cmake/ProjectSettings.cmake)

# Add targets with the helpers so none can forget the shared options and warnings:
#   add_project_library(<name> STATIC <sources...>)
#   add_project_executable(<name> <sources...>)
#   add_project_test(<name> <sources...>)   # GoogleTest, registered with ctest

add_subdirectory(src/base)

add_project_test(smoke_test tests/smoke_test.cpp)
if(BUILD_TESTING)
  add_subdirectory(tests/base)
endif()
