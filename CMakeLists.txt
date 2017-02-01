cmake_minimum_required(VERSION 3.6)
project(ArchiveLib)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c HashIndex.c HashIndex.h ArchivePage.c ArchivePage.h Archive.c Archive.h)
add_executable(ArchiveLib ${SOURCE_FILES})