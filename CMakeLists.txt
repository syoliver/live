cmake_minimum_required(VERSION 3.8)
project(Live)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 17)

add_executable(
  live
  
  src/main.cpp
  src/database.cpp
  src/json_timeseries.cpp
  src/http/server.cpp
  src/http/worker.cpp
)


target_compile_definitions(
  live
PRIVATE
  _HAS_DEPRECATED_RESULT_OF=1
  _WIN32_WINNT=0x0601
  BOOST_BEAST_USE_STD_STRING_VIEW
)

target_link_libraries(live ${CONAN_LIBS})
