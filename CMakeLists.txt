cmake_minimum_required(VERSION 2.8.12...3.29)

include_directories(duckdb/third_party/httplib)

set(CMAKE_DUCKDB_HTTPFS_BASE_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(extension/httpfs)
