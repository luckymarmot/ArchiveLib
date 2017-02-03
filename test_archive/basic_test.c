//
// Created by Matthaus Woolard on 02/02/2017.
//

#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>

#include <ArchivePage.h>
#include <Archive.h>

#include <cmocka.h>
#include <malloc/malloc.h>

static void test_ArchivePage(void **state) {
    assert_int_equal(sizeof(ArchivePage), 32);
}


static inline void rand_key(char* key)
{
    u_int32_t a = arc4random();
    u_int32_t b = arc4random();
    u_int32_t c = arc4random();
    u_int32_t d = arc4random();
    u_int32_t e = arc4random();
    key[0] = a & 0xff;
    key[1] = (a & 0xff00) >> 8;
    key[2] = (a & 0xff0000) >> 16;
    key[3] = (a & 0xff000000) >> 24;
    key[4 + 0] = b & 0xff;
    key[4 + 1] = (b & 0xff00) >> 8;
    key[4 + 2] = (b & 0xff0000) >> 16;
    key[4 + 3] = (b & 0xff000000) >> 24;
    key[8 + 0] = c & 0xff;
    key[8 + 1] = (c & 0xff00) >> 8;
    key[8 + 2] = (c & 0xff0000) >> 16;
    key[8 + 3] = (c & 0xff000000) >> 24;
    key[12 + 0] = d & 0xff;
    key[12 + 1] = (d & 0xff00) >> 8;
    key[12 + 2] = (d & 0xff0000) >> 16;
    key[12 + 3] = (d & 0xff000000) >> 24;
    key[16 + 0] = e & 0xff;
    key[16 + 1] = (e & 0xff00) >> 8;
    key[16 + 2] = (e & 0xff0000) >> 16;
    key[16 + 3] = (e & 0xff000000) >> 24;
}


/**
 * Test archive init
 */
static void test_Archive_init(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    assert_int_equal(archive.n_pages, 0);
    assert_null(archive.page_stack);
    assert_string_equal(archive.base_file_path, "./");

    // Test that the file name is copied.
    char* f = malloc(sizeof(char[3]));
    memcpy(f, "./", 3);
    Archive archive2;
    Archive_init(&archive2, f);
    free(f);
    assert_string_equal(archive2.base_file_path, "./");
}


/**
 *
 *  Test archive free
 */
static void test_Archive_free(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);
    assert_int_equal(archive.n_pages, 1);
    assert_non_null(archive.page_stack);
    assert_string_equal(archive.base_file_path, "./");

    ArchiveListItem* page_stack = archive.page_stack;
    ArchivePage* page = archive.page_stack->page;
    char* filename = page->filename;
    HashIndex* index = page->index;

    assert_int_not_equal(malloc_size(page), 0);
    assert_int_not_equal(malloc_size(page_stack), 0);
    assert_int_not_equal(malloc_size(filename), 0);
    assert_int_not_equal(malloc_size(index), 0);

    /////////// FREE THE ARCHIVE
    Archive_free(&archive);
    ////////// test that that it is free

    assert_int_equal(archive.n_pages, 0);

    assert_int_equal(malloc_size(page), 0);
    assert_int_equal(malloc_size(page_stack), 0);
    assert_int_equal(malloc_size(archive.page_stack), 0);
    assert_int_equal(malloc_size(archive.base_file_path), 0);

    assert_int_equal(malloc_size(filename), 0);
    assert_int_equal(malloc_size(index), 0);
}

/**
 *
 *  Test archive has
 */
static void test_Archive_has(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);

    char key[20] = {
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100
    };

    assert_false(Archive_has(&archive, key));

    HashIndex* index = archive.page_stack->page->index;
    HashIndex_set(index, key, 0, 0);

    assert_true(Archive_has(&archive, key));

    // Add lots of pages
    for (int i = 0; i < 5; ++i) {
        Archive_add_empty_page(&archive);
        assert_true(Archive_has(&archive, key));
    }

    // Lookup with more than 1 value in bucket
    char key2[20] = {
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 101
    };
    assert_false(Archive_has(&archive, key2));
    HashIndex_set(index, key2, 1, 1);
    assert_true(Archive_has(&archive, key2));

    char key3[20] = {
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 102
    };
    assert_false(Archive_has(&archive, key3));

    // Insect in the middle of the stack
    HashIndex_set(
            archive.page_stack->next->page->index,
            key3, 1, 1
    );

    assert_true(Archive_has(&archive, key3));

    /// Add a new page on 0
    char key4[20] = {
            0x00, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 102
    };
    assert_false(Archive_has(&archive, key4));
    HashIndex_set(
            archive.page_stack->next->page->index,
            key4, 1, 1
    );
    assert_true(Archive_has(&archive, key4));

    /// Add new page on 255
    /// Add a new page on 0
    char key5[20] = {
            (char) 0xFF, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 102
    };
    assert_false(Archive_has(&archive, key5));
    HashIndex_set(
            archive.page_stack->next->page->index,
            key5, 1, 1
    );
    assert_true(Archive_has(&archive, key5));
}



/**
 *
 *  Test archive has
 */
static void test_Archive_set(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    Archive_add_empty_page(&archive);

    char key[20] = {
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100, 100,
            100, 100, 100, 100, 100, 100
    };

    assert_false(Archive_has(&archive, key));


    char* data = "data";
    Errors error = Archive_set(&archive, key, data, 5);
    assert_int_equal(error, 0);
    assert_true(Archive_has(&archive, key));

    // Free this archive so we can open again
    char** filenames;
    char filename[10000];
    size_t _n_files;
    Archive_save(&archive, &filenames, &_n_files);
    strcpy(filename, filenames[0]);

    Archive_free(&archive);

    Archive archive2;
    Archive_init(&archive2, "./");
    Errors e = Archive_add_page_by_name(&archive2, filename+2);
    assert_int_equal(e, 0);

    assert_true(Archive_has(&archive2, key));

    char *data2;
    size_t data_size;
    e = Archive_get(&archive2, key, &data2, &data_size);
    assert_int_equal(e, 0);
    assert_string_equal(data2, "data");
    assert_int_equal(data_size, 5);
    Errors ef;
    // Test that set adds to a new page
    for (int i = 0; i < MAX_ITEMS_PER_INDEX * 10; ++i) {
        rand_key(key);
        ef = Archive_set(&archive2, key, "data", 5);
        assert_int_equal(ef, 0);
        assert_true(Archive_has(&archive2, key));
        ef = Archive_set(&archive2, key, "data", 5);
    }
    assert_int_equal(archive2.n_pages, 11);
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
            cmocka_unit_test(test_Archive_init),
            cmocka_unit_test(test_Archive_free),
            cmocka_unit_test(test_Archive_has),
            cmocka_unit_test(test_Archive_set)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}