package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "libmiaucam_mediapipe.so",
    srcs = [
        "miaucam_bridge.cc",
        "miaucam_bridge.h",
    ],
    linkshared = 1,
    # Hide everything pulled from static deps (protobuf, absl, tflite, ...)
    # so it can't collide with a *different* copy of the same libraries the
    # host process links separately (e.g. Ubuntu's libopencv_video.so drags
    # in the system libprotobuf via libopencv_dnn.so). Without this, both
    # protobuf runtimes fight over the same process-wide descriptor pool and
    # abort with "File already exists in database". protobuf's generated
    # code marks its symbols visibility("default") explicitly in source, so
    # -Wl,--exclude-libs,ALL (which only affects symbols that are default
    # visibility "by omission") can't hide them - a version script overrides
    # that at the linker level regardless. Only miaucam::* stays exported.
    additional_linker_inputs = [":miaucam_bridge.lds"],
    linkopts = ["-Wl,--version-script=$(location :miaucam_bridge.lds)"],
    deps = [
        "//mediapipe/tasks/cc/core:base_options",
        "//mediapipe/tasks/cc/vision/core:running_mode",
        "//mediapipe/tasks/cc/vision/face_landmarker",
        "//mediapipe/tasks/cc/vision/hand_landmarker",
        "@com_google_absl//absl/status:statusor",
    ],
)
