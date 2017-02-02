cmake_minimum_required(VERSION 3.6)


set(CMAKE_C_STANDARD 11)

add_library (Archive Archive.h Archive.c ArchivePage.c ArchivePage.h HashIndex.c HashIndex.h Errors.h )

add_executable(ArchiveLib main.c)
target_link_libraries (ArchiveLib Archive)

