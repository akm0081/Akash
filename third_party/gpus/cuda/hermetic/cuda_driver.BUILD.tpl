licenses(["restricted"])  # NVIDIA proprietary license

filegroup(
    name = "nvidia_driver",
    %{comment}srcs = glob([
      %{comment}"lib/libcuda.so*",
      %{comment}"lib/libcudadebugger.so*",
      %{comment}"lib/libnvidia-nvvm.so*",
      %{comment}"lib/libnvidia-pkcs11-openssl3.so*",
      %{comment}"lib/libnvidia-pkcs11.so*",
      %{comment}"lib/libnvidia-ptxjitcompiler.so*",
    %{comment}]),
    visibility = ["//visibility:public"],
)
