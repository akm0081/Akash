licenses(["restricted"])  # NVIDIA proprietary license

%{multiline_comment}
cc_import(
    name = "driver_shared_library",
    shared_library = "lib/libcuda.so.%{libcuda_version}",
)

cc_import(
    name = "driver_symlink",
    shared_library = "lib/libcuda.so.1",
)

cc_import(
    name = "driver_libcuda_symlink",
    shared_library = "lib/libcuda.so",
)

# This fake library is needed to add libcuda.so to RPATH.
cc_library(
    name = "fake_libcuda",
    srcs = ["libcuda.cc"],
    linkopts = ["-Wl,-soname,libcuda.so"],
)
%{multiline_comment}
cc_library(
    name = "nvidia_driver",
    %{comment}deps = [
        %{comment}":fake_libcuda",
        %{comment}":driver_libcuda_symlink",
        %{comment}":driver_symlink",
        %{comment}":driver_shared_library",
    %{comment}],
    visibility = ["//visibility:public"],
)
