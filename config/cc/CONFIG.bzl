load("@obazl_config_cc//config:BASE.bzl",
     _BASE_COPTS    = "BASE_COPTS",
     _define_module_version = "define_module_version")

define_module_version = _define_module_version
BASE_COPTS = _BASE_COPTS + select({
    ## macos:
    "@obazl_config_cc//profile:dev?": [
        "-Wno-gnu-statement-expression"],
    "//conditions:default": []
})
BASE_LINKOPTS = []
PROFILE       = ["PROFILE_$(COMPILATION_MODE)"]

