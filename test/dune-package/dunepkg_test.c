#include <stdio.h>
#include "libs7.h"
#include "unity.h"

#include "include_test.h"

int test_main(int argc, char **argv);
int main(int argc, char **argv)
{
    return test_main(argc, argv);
}

void test_runner(void)
{
    s7_int gc_expected = -1;
    data_fname_str = "test/dune-package/case" CASE "/dune-package";
    data_expected = read_expected("test/dune-package/case" CASE "/sexp.expected");
    gc_expected = s7_gc_protect(s7, data_expected);
    RUN_TEST(test_read_file_port);
    /* RUN_TEST(test_with_input_from_file); */
    /* RUN_TEST(test_call_with_input_file); */
    s7_gc_unprotect_at(s7, gc_expected);
}
