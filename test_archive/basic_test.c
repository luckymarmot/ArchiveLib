//
// Created by Matthaus Woolard on 02/02/2017.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include "../archive/Archive.h"

/* A test_archive case that does nothing and succeeds. */
static void null_test_success(void **state) {
    Archive archive;


    (void) state; /* unused */
}

int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(null_test_success),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}