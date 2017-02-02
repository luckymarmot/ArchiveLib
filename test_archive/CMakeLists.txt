project(archive_tests)

set(SOURCE_FILES basic_test.c)
add_executable(archive_tests ${SOURCE_FILES})
target_link_libraries(archive_tests cmocka archive)
include_directories ("${PROJECT_SOURCE_DIR}/archive")
add_test(archive_tests archive_tests)
