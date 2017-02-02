cmake_minimum_required(VERSION 3.6)
project(ArchiveLib)
set(CMAKE_C_STANDARD 11)

add_subdirectory(test_archive)
add_subdirectory(archive)

enable_testing()
include_directories(archive)


