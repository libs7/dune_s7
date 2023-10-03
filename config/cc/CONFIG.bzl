load("@cc_config//:CONFIG.bzl",
     _BASE_COPTS    = "BASE_COPTS",
     _BASE_LINKOPTS = "BASE_LINKOPTS")

BASE_COPTS = _BASE_COPTS + select({
    ## macos:
    "//config/compilation_mode:fastbuild?": [
        "-Wno-gnu-statement-expression"],
    "//conditions:default": []
})
BASE_LINKOPTS = _BASE_LINKOPTS
BASE_DEFINES = ["DEBUG_$(COMPILATION_MODE)"]

BASE_SRCS = []
BASE_DEPS = []
BASE_INCLUDE_PATHS = []