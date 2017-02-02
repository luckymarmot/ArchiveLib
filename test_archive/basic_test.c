//
// Created by Matthaus Woolard on 02/02/2017.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <Archive.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>


/* A test_archive case that does nothing and succeeds. */
static void test_archive_init(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    assert_int_equal(archive.n_pages, 0);
    assert_string_equal(archive.base_file_path, "./");
    assert_null(archive.page_stack);
    Archive_free(&archive);
}


static void test_archive_new_file(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_new_page(&archive);
    assert_int_equal(archive.n_pages, 1);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    void* pstack1 = archive.page_stack;
    assert_null(archive.page_stack->next);

    // Add a new page
    Archive_new_page(&archive);
    assert_int_equal(archive.n_pages, 2);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    assert_ptr_not_equal(pstack1, archive.page_stack);
    // the new page linked list should point to the old linked list
    assert_ptr_equal(archive.page_stack->next, pstack1);
    void* pstack2 = archive.page_stack;

    // Add a new page
    Archive_new_page(&archive);
    assert_int_equal(archive.n_pages, 3);
    assert_non_null(archive.page_stack);
    assert_ptr_not_equal(pstack2, archive.page_stack);
    // the new page linked list should point to the old linked list
    assert_ptr_equal(archive.page_stack->next, pstack2);

    Archive_free(&archive);
}


static void test_archive_new_file_created_on_init(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_new_page(&archive);
    assert_int_equal(archive.n_pages, 1);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    assert_null(archive.page_stack->next);

    FILE* f = fopen(archive.page_stack->page->filename, "r+");
    char* data = (char*) malloc(sizeof(char)*10);
    size_t result = fread(data, 1, 10, f);

    unsigned short version = 0;
    size_t header_start = sizeof(ArchiveFileHeader);
    assert_memory_equal(data, &version, sizeof(unsigned short));
    assert_memory_equal(data + sizeof(unsigned short), &header_start, sizeof(size_t));

    assert_int_equal(result, 10);

    fclose(f);

    Archive_free(&archive);
}


static void test_archive_file_close_on_free(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_new_page(&archive);

    // There should be a lock on the file
    int response = lockf(archive.page_stack->page->fd, F_TEST, 0);
    assert_int_equal(response, -1);
    Archive_free(&archive);

    // There should be no lock on the file! yay
    response = lockf(archive.page_stack->page->fd, F_TEST, 0);
    assert_int_equal(response, 0);
}


int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_archive_init),
            cmocka_unit_test(test_archive_new_file),
            cmocka_unit_test(test_archive_new_file_created_on_init),
            cmocka_unit_test(test_archive_file_close_on_free)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}