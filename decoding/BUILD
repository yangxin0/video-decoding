package(default_visibility = ["//visibility:public"])

cc_library(
    name = "video_decoding_lib",
    srcs = ["video_decoding.cpp", "video_decoding.h"],
    deps = ["@libavformat//:shared_lib", "@libavcodec//:shared_lib"]
)

cc_binary(
    name = "video_decoding",
    srcs = ["video_decoding_main.cpp"],
    deps = [":video_decoding_lib"],
)
