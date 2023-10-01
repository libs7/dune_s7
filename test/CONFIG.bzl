load("@libs7//plugin:CONFIG.bzl",
     "PLUGIN_TEST_DEPS",
     "PLUGIN_TEST_INCLUDE_PATHS")

TEST_SRCS  = ["//test:macros.h", "//src:trace_dev.h"]

TEST_DEPS  = ["//src:dune_s7"] + PLUGIN_TEST_DEPS

TEST_INCLUDE_PATHS = [
    "-I$(@)/src",
    "-I$(@)/test"
] + PLUGIN_TEST_INCLUDE_PATHS

TEST_TOOLCHAINS = ["//:test_repo_paths"]
TEST_TIMEOUT = "short"
TEST_TAGS = ["sexp"]
