cmake_minimum_required(VERSION 3.14)
project(highload CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)
 
find_package(userver COMPONENTS core QUIET)
if(NOT userver_FOUND)
  # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
  download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

add_library(${PROJECT_NAME}_objs OBJECT
    src/say_hello.hpp
    src/say_hello.cpp
    src/hello_handler.hpp
    src/hello_handler.cpp
)
target_link_libraries(${PROJECT_NAME}_objs userver::core)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)
# 
 
# 
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_objs)
# 
 
# 
add_executable(${PROJECT_NAME}-unittest unittests/say_hello_test.cpp)
target_link_libraries(${PROJECT_NAME}-unittest ${PROJECT_NAME}_objs userver::utest)
add_google_tests(${PROJECT_NAME}-unittest)
# 
 
# 
add_executable(${PROJECT_NAME}_benchmark benchmarks/say_hello_bench.cpp)
target_link_libraries(${PROJECT_NAME}_benchmark ${PROJECT_NAME}_objs userver::ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)
# 
 
# 
# Makes a virtualenv suitable for running pytests from `test` directory and integrates with `ctest`.
userver_testsuite_add_simple()
# 