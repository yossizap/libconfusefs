cmake_minimum_required(VERSION 3.10)
project(libConfFS VERSION 0.1.0 LANGUAGES CXX)

LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")

#option(LIBCONFUSEFS_USE_EXTERNAL_JSON "Use an external JSON library" OFF)

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(test)
