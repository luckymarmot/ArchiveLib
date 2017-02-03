//
// Created by Matthaus Woolard on 02/02/2017.
//

#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>

#include <ArchivePage.h>
#include <Archive.h>

#include <cmocka.h>


static void test_ArchivePage(void **state) {
    assert_int_equal(sizeof(ArchivePage), 32);
}


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
    Archive_add_empty_page(&archive);
    assert_int_equal(archive.n_pages, 1);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    void* pstack1 = archive.page_stack;
    assert_null(archive.page_stack->next);

    // Add a new page
    Archive_add_empty_page(&archive);
    assert_int_equal(archive.n_pages, 2);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    assert_ptr_not_equal(pstack1, archive.page_stack);
    // the new page linked list should point to the old linked list
    assert_ptr_equal(archive.page_stack->next, pstack1);
    void* pstack2 = archive.page_stack;

    // Add a new page
    Archive_add_empty_page(&archive);
    assert_int_equal(archive.n_pages, 3);
    assert_non_null(archive.page_stack);
    assert_ptr_not_equal(pstack2, archive.page_stack);
    // the new page linked list should point to the old linked list
    assert_ptr_equal(archive.page_stack->next, pstack2);

    Archive_free(&archive);
}


static void test_ArchivePage_init_saves_head(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);
    assert_int_equal(archive.n_pages, 1);
    assert_string_equal(archive.base_file_path, "./");
    assert_non_null(archive.page_stack);
    assert_null(archive.page_stack->next);

    FILE* f = fopen(archive.page_stack->page->filename, "r+");
    char* data = (char*) malloc(sizeof(char)*10);
    size_t result = fread(data, 1, 10, f);

    unsigned short version = 0;
    assert_memory_equal(data, &version, sizeof(unsigned short));


    fclose(f);

    Archive_free(&archive);
}


static void test_ArchivePage_open_file_locks_file(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);

    // There should be a lock on the file
    int response = lockf(archive.page_stack->page->fd, F_TEST, 0);
    assert_int_equal(response, -1);
    file_descriptor fd = open(archive.page_stack->page->filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    Archive_free(&archive);

    // There should be no lock on the file! yay
    response = lockf(fd, F_TEST, 0);
    assert_int_equal(response, 0);
}



/**
 *
 * Test init preps index
 */
static void test_HashIndex_init(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);
    HashIndex* index = archive.page_stack->page->index;
    assert_non_null(index);
    assert_int_equal(index->n_items, 0);
    for (int i = 0; i < 256; ++i) {
        assert_null(index->pages[i]);
    }
    Archive_free(&archive);

    HashIndex indexL;
    HashIndex_init(&indexL);
    assert_int_equal(indexL.n_items, 0);
    for (int i = 0; i < 256; ++i) {
        assert_null(indexL.pages[i]);
    }

}





int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_archive_init),
            cmocka_unit_test(test_archive_new_file),
            cmocka_unit_test(test_ArchivePage_init_saves_head),
            cmocka_unit_test(test_ArchivePage_open_file_locks_file),
            cmocka_unit_test(test_HashIndex_init),
            cmocka_unit_test(test_ArchivePage),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}