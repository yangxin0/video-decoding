load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_pkg_config",
    strip_prefix = "bazel_pkg_config-main",
    urls = ["https://github.com/yangxin0/bazel_pkg_config/archive/refs/heads/main.zip"],
    sha256 = "6f1041545e29f5c2419fd1655f928c34f99f248e7a4e886d1c7044f67d6c7940"
)

load("@bazel_pkg_config//:pkg_config.bzl", "pkg_config_shared")

pkg_config_shared(name = "libavformat")
pkg_config_shared(name = "libavcodec")
