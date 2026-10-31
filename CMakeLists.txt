cmake_minimum_required(VERSION 3.28)
# C: glad's generated loader and GLFW are C.
project(gl_engine LANGUAGES C CXX)

# Compiler floor, warnings, sanitizers, GoogleTest and the add_project_* helpers.
include(cmake/ProjectSettings.cmake)
# Pinned third-party libraries: glm, glad2, GLFW, Dear ImGui, stb.
include(cmake/Dependencies.cmake)

# Add targets with the helpers so none can forget the shared options and warnings:
#   add_project_library(<name> STATIC <sources...>)
#   add_project_executable(<name> <sources...>)
#   add_project_test(<name> <sources...>)   # GoogleTest, registered with ctest

add_subdirectory(src/base)

add_project_test(smoke_test tests/smoke_test.cpp)
if(BUILD_TESTING)
  add_subdirectory(tests/base)
endif()
