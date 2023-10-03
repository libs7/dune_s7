#include "gopt.h"
#include "unity.h"
#include "libs7.h"

#include "test_functions.h"
#include "macros.h"
#include "s7plugin_test_config.h"

extern struct option options[];

// fn syms, initialized by main
s7_pointer sexp_read;
s7_pointer libs7_read; //plain ol' read
s7_pointer with_input_from_file;
s7_pointer call_with_input_file;

char *sexp_str;

s7_pointer flag;
s7_pointer actual; // set by each test
s7_pointer expected; // initialized by main

char *cmd;

const char *data_fname_str;
s7_pointer data_fname7;
s7_pointer expected_fname7;

s7_pointer baddot_expected;

bool verbose;
bool debug;

int main(int argc, char **argv)
{
    s7 = s7_plugin_initialize("baddot", argc, argv);

    libs7_load_plugin(s7, "dune");

    init_unity(s7);

    s7_int gc_expected = -1;

    UNITY_BEGIN();

    data_fname_str = "test/baddot/case010/dune";
    baddot_expected = read_expected("test/baddot/case010/sexp.expected");
    gc_expected = s7_gc_protect(s7, baddot_expected);
    RUN_TEST(test_read_file_port);
    RUN_TEST(test_with_input_from_file);
    RUN_TEST(test_call_with_input_file);
    s7_gc_unprotect_at(s7, gc_expected);

    data_fname_str = "test/baddot/case020/dune";
    baddot_expected = read_expected("test/baddot/case020/sexp.expected");
    gc_expected = s7_gc_protect(s7, baddot_expected);
    RUN_TEST(test_read_file_port);
    RUN_TEST(test_with_input_from_file);
    RUN_TEST(test_call_with_input_file);
    s7_gc_unprotect_at(s7, gc_expected);

    data_fname_str = "test/baddot/case030/dune";
    baddot_expected = read_expected("test/baddot/case030/sexp.expected");
    gc_expected = s7_gc_protect(s7, baddot_expected);
    RUN_TEST(test_read_file_port);
    RUN_TEST(test_with_input_from_file);
    RUN_TEST(test_call_with_input_file);
    s7_gc_unprotect_at(s7, gc_expected);

    s7_quit(s7);
    s7_free(s7);
    return UNITY_END();
}
