#!/usr/bin/env bash
# Copyright 2024 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
#
#
# This script combines cuda_compat.sh and parallel_gpu_execute.sh. Both
# scripts are intended to be used with Bazel's `--run_under` feature which
# doesn't support composition.

# Depending on the repository (XLA or TF) the paths are different
function find_script {
  # `rlocation` is a function injected by Bazel which helps finding runfile 
  # locations.
  SCRIPT="$(rlocation xla/$1)"

  if [ -z "${SCRIPT}" ]; then
    SCRIPT="$(rlocation org_tensorflow/tensorflow/$1)"
  fi

  echo "${SCRIPT}"
}

exec "$(find_script tools/ci_build/gpu_build/cuda_compat.sh)" "$(find_script tools/ci_build/gpu_build/parallel_gpu_execute.sh)" "$@"