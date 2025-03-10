#include <stdlib.h>

#include "gopt.h"
#include "unity.h"
#include "libs7.h"

#include "macros.h"
#include "s7plugin_test_config.h"

#include "test_main.h"

s7_scheme *s7;

/* extern struct option options[]; */

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

s7_pointer data_expected;

#if defined(PROFILE_fastbuild)
#define     TRACE_FLAG trace_dune_s7
extern bool TRACE_FLAG;
#define     DEBUG_LEVEL debug_dune_s7
extern int  DEBUG_LEVEL;
extern int  s7plugin_debug;
#endif
bool verbose;
int  verbosity;
int  libs7_verbosity;

/* static void dunes7_print_usage(char *test) { */
/*     printf("Usage:\t$ bazel test test:%s [-- flags]\n", test); */
/*     printf("  Flags (repeatable)\n"); */
/*     printf("\t-d, --debug\t\tEnable debugging flags.\n"); */
/*     printf("\t-t, --trace\t\tEnable trace flags.\n"); */
/*     printf("\t-v, --verbose\t\tEnable verbosity. Repeatable.\n"); */
/*     printf("\t    --plugin-debug\tEnable plugin debugging flags.\n"); */
/* } */

/* enum OPTS { */
/*     FLAG_HELP, */
/* #if defined(PROFILE_fastbuild) */
/*     FLAG_DEBUG, */
/*     FLAG_DEBUG_PLUGINS, */
/*     FLAG_TRACE, */
/* #endif */
/*     FLAG_VERBOSE, */
/*     LAST */
/* }; */

/* struct option options[] = { */
/*     /\* 0 *\/ */
/* #if defined(PROFILE_fastbuild) */
/*     [FLAG_DEBUG] = {.long_name="debug", .short_name='d', */
/*                     .flags=GOPT_ARGUMENT_FORBIDDEN | GOPT_REPEATABLE}, */
/*     [FLAG_DEBUG_PLUGINS] = {.long_name="plugin-debug", */
/*                     .flags=GOPT_ARGUMENT_FORBIDDEN | GOPT_REPEATABLE}, */
/*     [FLAG_TRACE] = {.long_name="trace",.short_name='t', */
/*                     .flags=GOPT_ARGUMENT_FORBIDDEN}, */
/* #endif */
/*     [FLAG_VERBOSE] = {.long_name="verbose",.short_name='v', */
/*                       .flags=GOPT_ARGUMENT_FORBIDDEN | GOPT_REPEATABLE}, */
/*     [FLAG_HELP] = {.long_name="help",.short_name='h', */
/*                    .flags=GOPT_ARGUMENT_FORBIDDEN}, */
/*     [LAST] = {.flags = GOPT_LAST} */
/* }; */

/* void set_options(char *test, struct option options[]) */
/* { */
/*     /\* log_trace("set_options"); *\/ */
/*     if (options[FLAG_HELP].count) { */
/*         dunes7_print_usage(test); */
/*         exit(EXIT_SUCCESS); */
/*     } */
/* #if defined(PROFILE_fastbuild) */
/*     if (options[FLAG_DEBUG].count) { */
/*         debug_dune_s7 = options[FLAG_DEBUG].count; */
/*     } */
/*     if (options[FLAG_DEBUG_PLUGINS].count) { */
/*         s7plugin_debug = options[FLAG_DEBUG_PLUGINS].count; */
/*     } */
/*     if (options[FLAG_TRACE].count) { */
/*         trace_dune_s7 = true; */
/*     } */
/* #endif */
/*     if (options[FLAG_VERBOSE].count) { */
/*         verbosity = options[FLAG_VERBOSE].count; */
/*         LOG_INFO(0, "verbosity: %d", verbosity); */
/*         verbose = true; */
/*         if (verbosity > 1) { */
/*             libs7_verbosity = verbosity; */
/*         } */
/*     } */
/* } */

int test_main(int argc, char **argv)
{
    /* argc = gopt (argv, options); */
    /* (void)argc; */
    /* gopt_errors (argv[0], options); */

    /* set_options("dune_s7", options); */

    s7 = s7_plugin_initialize("dune_s7", argc, argv);

    libs7_load_plugin(s7, "dune");

    debug_dune_s7 = 1;
    trace_dune_s7 = true;
    libs7_debug = 1;

    init_unity(s7);

    UNITY_BEGIN();

    test_runner(); /* defined in each test */

    /* s7_flush_output_port(s7, s7_current_output_port(s7)); */
    /* s7_flush_output_port(s7, s7_current_error_port(s7)); */
    /* fflush(NULL); */

    s7_quit(s7);                /* exit interpreter */
    s7_free(s7);

    return UNITY_END();
}
