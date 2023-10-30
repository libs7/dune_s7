load("@obazl_tools_cc//config:BASE.bzl",
     _BASE_COPTS    = "BASE_COPTS",
     _BASE_LINKOPTS = "BASE_LINKOPTS",
     _define_module_version = "define_module_version")

define_module_version = _define_module_version
BASE_COPTS = _BASE_COPTS + select({
    ## macos:
    "//config/compilation_mode:fastbuild?": [
        "-Wno-gnu-statement-expression"],
    "//conditions:default": []
})
BASE_LINKOPTS = _BASE_LINKOPTS
PROFILE       = ["PROFILE_$(COMPILATION_MODE)"]

