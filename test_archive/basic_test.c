//
// Created by Matthaus Woolard on 02/02/2017.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <Archive.h>

/* A test_archive case that does nothing and succeeds. */
static void test_archive_init(void **state) {
    Archive archive;
    Archive_init(&archive, "./");
    assert_int_equal(archive.n_pages, 0);
    Archive_free(&archive);
}



int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_archive_init),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}