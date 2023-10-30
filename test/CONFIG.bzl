load("@libs7//plugin:CONFIG.bzl",
     "PLUGIN_TEST_DEPS",
     "PLUGIN_TEST_INCLUDE_PATHS")

TEST_SRCS  = ["//test:macros.h"] #, "//lib:macros_debug.h"]

TEST_DEPS  =  PLUGIN_TEST_DEPS + [
    "//lib:dune_s7",
    "//test:test_functions"
]

TEST_INCLUDE_PATHS = [
    # "-I$(@)/src",
    # "-I$(@)/test"
] + PLUGIN_TEST_INCLUDE_PATHS

TEST_TOOLCHAINS = ["//:test_module_profiles"]
TEST_TIMEOUT = "short"
TEST_TAGS = ["sexp"]
