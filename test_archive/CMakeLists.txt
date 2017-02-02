
include_directories(../archive)

set(SOURCE_FILES basic_test.c)
add_executable(archive_tests ${SOURCE_FILES})
target_link_libraries(archive_tests cmocka Archive)
add_test(archive_tests archive_tests)
