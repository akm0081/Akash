/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_XLA_RUNTIME_ERRORS_H_
#define TENSORFLOW_COMPILER_XLA_RUNTIME_ERRORS_H_

#include <exception>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include <iostream>
namespace xla {
namespace runtime {

template <typename... Args>
absl::Status InvalidArgument(const absl::FormatSpec<Args...>& format,
                             const Args&... args) {
  return absl::InvalidArgumentError(absl::StrFormat(format, args...));
}

template <typename... Args>
absl::Status InternalError(const absl::FormatSpec<Args...>& format,
                           const Args&... args) {
  return absl::InternalError(absl::StrFormat(format, args...));
}

/*
class XlaError : public std::exception {
protected:
    char *message;
public:
    XlaError(char *msg) : message(msg) {}
    virtual const char * what () {
        return message;
    }
};

class XlaInvalidArgumentError : public XlaError {
public:
    XlaInvalidArgumentError(char *msg) : XlaError(msg) {}
    virtual const char * what () {
        return message;
    }
};
*/

}  // namespace runtime
}  // namespace xla

#endif  // TENSORFLOW_COMPILER_XLA_RUNTIME_ERRORS_H_
