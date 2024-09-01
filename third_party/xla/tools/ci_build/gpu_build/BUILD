# Description:
# TensorFlow is a computational framework, primarily for use in machine
# learning applications.

load("//xla/tsl:tsl.bzl", "if_oss")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    licenses = ["notice"],
)

sh_binary(
    name = "parallel_gpu_execute",
    srcs = ["parallel_gpu_execute.sh"],
)

sh_binary(
    name = "cuda_compat",
    srcs = ["cuda_compat.sh"],
    data = if_oss(["@cuda_driver//:nvidia_driver"]),
)

sh_binary(
    name = "cuda_compat_and_parallel_gpu_execute",
    srcs = ["cuda_compat_and_parallel_gpu_execute.sh"],
    data = [
        ":cuda_compat",
        ":parallel_gpu_execute",
    ],
)

filegroup(
    name = "all_files",
    srcs = glob(
        ["**/*"],
        exclude = [
            "**/METADATA",
            "**/OWNERS",
        ],
    ),
    visibility = ["//visibility:private"],
)
