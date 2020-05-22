cmake_minimum_required(VERSION 3.8)
project(Live)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(live main.cpp)
target_link_libraries(md5 ${CONAN_LIBS})
