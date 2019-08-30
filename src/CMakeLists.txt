cmake_minimum_required(VERSION 3.10)
project(libConfuseFS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-O3 -Wall -Wextra -pedantic -Wrestrict \
    -Wlogical-op -Wduplicated-branches -Wduplicated-cond -Wnull-dereference
    -Wuseless-cast -Wdouble-promotion -Wshadow -Wformat=2")

find_package(FUSE3 REQUIRED)

set(LIBCONFUSEFS_PUBLIC_HEADERS confusefs.hpp)

add_library(libConfuseFS
    confusefs.cpp
    ${LIBCONFUSEFS_PUBLIC_HEADERS})
add_library(libConfuseFS::libConfuseFS ALIAS libConfuseFS)

target_include_directories(
    libConfuseFS PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(libConfuseFS
    PROPERTIES
    PUBLIC_HEADER ${LIBCONFUSEFS_PUBLIC_HEADERS})

target_link_libraries(
    libConfuseFS
    PUBLIC json-schema-validator nlohmann_json FUSE3 JConf::JConf)
