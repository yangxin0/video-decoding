package(default_visibility = ["//visibility:private"])

cc_import(
    name = "%{external_pkg_name}",
    shared_library = "%{external_lib}"
)

cc_library(
    name = "shared_lib",
    hdrs = glob(["%{external_header_dir}/**/*.h"]),
    deps = ["%{external_pkg_name}"],
    strip_include_prefix = "%{external_header_dir}",
    visibility = ["//visibility:public"],
)