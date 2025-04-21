cmake_minimum_required(VERSION 3.14)
project(highload CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)
 
find_package(userver COMPONENTS core postgresql QUIET)
if(NOT userver_FOUND)
  # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
  download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} userver::postgresql)
 
# 
userver_add_sql_library(
    ${PROJECT_NAME}_sql
    NAMESPACE samples_postgres_service
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SQL_FILES *.sql
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_sql)
# 
 
# 
userver_testsuite_add_simple(SQL_LIBRARY ${PROJECT_NAME}_sql)
# 