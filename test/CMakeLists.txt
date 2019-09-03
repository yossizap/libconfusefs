enable_testing()

find_package(FUSE3 REQUIRED)
find_package(LibUV REQUIRED)

add_executable(ConfuseFSTest test.cpp)
target_link_libraries(ConfuseFSTest PRIVATE libConfuseFS::libConfuseFS uv pthread)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-O3 -Wall -pedantic")

add_custom_command(TARGET ConfuseFSTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_LIST_DIR}/../third_party/JConf/test/config $<TARGET_FILE_DIR:ConfuseFSTest>)
