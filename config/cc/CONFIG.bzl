load("@cc_config//:CONFIG.bzl",
     _BASE_COPTS    = "BASE_COPTS",
     _BASE_LINKOPTS = "BASE_LINKOPTS")

BASE_COPTS = _BASE_COPTS + select({
    "//config/profile:dev?": ["-Wno-gnu-statement-expression"],
    "//conditions:default": []
})
BASE_LINKOPTS = _BASE_LINKOPTS
BASE_DEFINES = []

BASE_SRCS = []
BASE_DEPS = []
BASE_INCLUDE_PATHS = []
