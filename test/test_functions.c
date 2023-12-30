/* #include "gopt.h" */
#include "unity.h"
#include "libs7.h"

#include "test_functions.h"
#include "macros.h"

#if defined(PROFILE_fastbuild)
#define TRACE_FLAG  dune_s7_trace
#define DEBUG_LEVEL dune_s7_debug
extern bool    TRACE_FLAG;
extern int     DEBUG_LEVEL;

#define S7_DEBUG_LEVEL libs7_debug
extern int  libs7_debug;
extern bool s7plugin_trace;
extern int  s7plugin_debug;
#endif

/* extern struct option options[]; */

// fn syms, initialized by main
s7_pointer dune_read;
s7_pointer libs7_read; //plain ol' read
s7_pointer with_input_from_file;
s7_pointer call_with_input_file;

/* char *sexp_str; */

s7_pointer flag;
s7_pointer actual; // set by each test
s7_pointer expected; // initialized by main

char *cmd;

const char *data_fname_str;
s7_pointer data_fname7;
s7_pointer expected_fname7;

s7_pointer data_expected;

#if defined(PROFILE_fastbuild)
#define     TRACE_FLAG dune_s7_trace
extern bool TRACE_FLAG;
#define     DEBUG_LEVEL dune_s7_debug
extern int  DEBUG_LEVEL;
extern int  s7plugin_debug;
#endif
bool verbose;

s7_scheme *s7;

void setUp(void) {}

void tearDown(void) {}

void init_unity(s7_scheme *_s7) {
    s7 = _s7;
    dune_read = s7_name_to_value(s7, "dune:read");
    libs7_read = s7_name_to_value(s7, "read");

    with_input_from_file = s7_name_to_value(s7, "with-input-from-file");
    call_with_input_file = s7_name_to_value(s7, "call-with-input-file");
}

void test_read_file_port(void) {
    s7_pointer inport = s7_open_input_file(s7, data_fname_str,  "r");
    TEST_ASSERT_TRUE(s7_is_input_port(s7, inport));
    actual = s7_apply_function(s7, dune_read, s7_list(s7, 1, inport));
    flag = APPLY_1("alist?", actual);
    TEST_ASSERT_TRUE(s7_boolean(s7, flag));
    s7_close_input_port(s7, inport);

    TRACE_S7_DUMP(0, "actual: %s", actual);
    TRACE_S7_DUMP(0, "expected: %s", data_expected);

    flag = APPLY_2("equal?", actual, data_expected);
    TEST_ASSERT_TRUE_MESSAGE(s7_boolean(s7, flag), data_fname_str);
}

void test_with_input_from_file(void) {
    data_fname7 = s7_make_string(s7, data_fname_str);
    s7_pointer readlet
        = s7_inlet(s7,
                   s7_list(s7, 1,
                           s7_cons(s7,
                                   s7_make_symbol(s7, "datafile"),
                                   data_fname7)));

    cmd = "(with-input-from-file datafile dune:read)";
    actual = s7_eval_c_string_with_environment(s7, cmd, readlet);
    flag = APPLY_1("alist?", actual);
    TEST_ASSERT_TRUE(s7_boolean(s7, flag));
    TRACE_S7_DUMP(0, "actual: %s", actual);
    TRACE_S7_DUMP(0, "expected: %s", data_expected);
    flag = APPLY_2("equal?", actual, data_expected);
    TEST_ASSERT_TRUE_MESSAGE(s7_boolean(s7, flag), data_fname_str);
}

void test_call_with_input_file(void) {
    data_fname7 = s7_make_string(s7, data_fname_str);
    s7_pointer readlet
        = s7_inlet(s7,
                   s7_list(s7, 1,
                           s7_cons(s7,
                                   s7_make_symbol(s7, "datafile"),
                                   data_fname7)));

    cmd = "(call-with-input-file datafile dune:read)";
    actual = s7_eval_c_string_with_environment(s7, cmd, readlet);
    flag = APPLY_1("alist?", actual);
    TEST_ASSERT_TRUE(s7_boolean(s7, flag));
    TRACE_S7_DUMP(0, "actual: %s", actual);
    TRACE_S7_DUMP(0, "expected: %s", data_expected);
    flag = APPLY_2("equal?", actual, data_expected);
    TEST_ASSERT_TRUE_MESSAGE(s7_boolean(s7, flag), data_fname_str);
}

s7_pointer read_expected(char *fname) {
    s7_pointer fname7 = s7_make_string(s7, fname);
    s7_pointer readlet
        = s7_inlet(s7,
                   s7_list(s7, 1,
                           /* s7_cons(s7, */
                           /*         s7_make_symbol(s7, "datafile"), */
                           /*         baddot_010_data_fname7), */
                           s7_cons(s7,
                                   s7_make_symbol(s7, "expected"),
                                   fname7)));

    /* cmd = "(with-input-from-file datafile dune:read)"; */
    /* actual = s7_eval_c_string_with_environment(s7, cmd, readlet); */
    /* TRACE_S7_DUMP(0, "actual: %s", actual); */

    // use read not dune:read for expected
    cmd = "(with-input-from-file expected read)";
    return s7_eval_c_string_with_environment(s7, cmd, readlet);
}
