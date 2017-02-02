cmake_minimum_required(VERSION 3.6)
project(ArchiveLib)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES archive/main.c archive/HashIndex.c archive/HashIndex.h archive/ArchivePage.c archive/ArchivePage.h archive/Archive.c archive/Archive.h)
add_executable(ArchiveLib ${SOURCE_FILES})