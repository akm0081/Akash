/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

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

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tensorflow/lite/c/c_api_opaque.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/core/c/c_api_types.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/interpreter_builder.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/testing/util.h"

using ::testing::ContainsRegex;
namespace tflite {
namespace delegates {

namespace {

TEST(TestOpaqueDelegate, AddDelegate) {
  std::unique_ptr<tflite::FlatBufferModel> model =
      tflite::FlatBufferModel::BuildFromFile(
          "tensorflow/lite/testdata/add.bin");
  ASSERT_NE(model, nullptr);

  TfLiteOpaqueDelegateBuilder opaque_delegate_builder{};
  opaque_delegate_builder.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                       TfLiteOpaqueDelegate* opaque_delegate,
                                       void* data) -> TfLiteStatus {
    // Test that an unnamed delegate kernel can be passed to the TF Lite
    // runtime.
    TfLiteRegistrationExternal* registration_external =
        TfLiteRegistrationExternalCreate(kTfLiteBuiltinDelegate,
                                         /*name*/ nullptr,
                                         /*version=*/1);
    TfLiteRegistrationExternalSetInit(
        registration_external,
        [](TfLiteOpaqueContext* context, const char* buffer,
           size_t length) -> void* { return nullptr; });
    TfLiteIntArray* execution_plan;
    TF_LITE_ENSURE_STATUS(
        TfLiteOpaqueContextGetExecutionPlan(opaque_context, &execution_plan));
    return TfLiteOpaqueContextReplaceNodeSubsetsWithDelegateKernels(
        opaque_context, registration_external, execution_plan, opaque_delegate);
  };
  TfLiteOpaqueDelegate* opaque_delegate =
      TfLiteOpaqueDelegateCreate(&opaque_delegate_builder);

  tflite::ops::builtin::BuiltinOpResolver resolver;
  tflite::InterpreterBuilder builder(*model, resolver);
  builder.AddDelegate(opaque_delegate);
  std::unique_ptr<tflite::Interpreter> interpreter;
  EXPECT_EQ(kTfLiteOk, builder(&interpreter));
  ASSERT_NE(interpreter, nullptr);
  TfLiteOpaqueDelegateDelete(opaque_delegate);
}

class TestOpaqueMacros : public ::testing::Test {
  void DelegateImpl(TfLiteStatus expected_status) {
    // The 'opaque_delegate_builder_' is being configured in the test itself.
    opaque_delegate_ = TfLiteOpaqueDelegateCreate(&opaque_delegate_builder_);
    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model_, resolver);
    builder.AddDelegate(opaque_delegate_);
    std::unique_ptr<tflite::Interpreter> interpreter;
    EXPECT_EQ(expected_status, builder(&interpreter));
  }
  void SetUp() override {
    model_ = tflite::FlatBufferModel::BuildFromFile(
        "tensorflow/lite/testdata/add.bin", &reporter_);
    ASSERT_NE(model_, nullptr);
  }
  void TearDown() override { TfLiteOpaqueDelegateDelete(opaque_delegate_); }

 public:
  TestOpaqueMacros()
      : reporter_{},
        model_{},
        opaque_delegate_builder_{},
        opaque_delegate_(nullptr) {}
  void EnsureDelegationSucceeds() { DelegateImpl(kTfLiteOk); }
  void EnsureDelegationFails() { DelegateImpl(kTfLiteDelegateError); }
  ::tflite::TestErrorReporter reporter_;
  std::unique_ptr<tflite::FlatBufferModel> model_;
  TfLiteOpaqueDelegateBuilder opaque_delegate_builder_;
  TfLiteOpaqueDelegate* opaque_delegate_;
};

TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE(opaque_context, false);
    return kTfLiteOk;
  };
  EnsureDelegationFails();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*false was not true.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE(opaque_context, true);
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*was not true.*");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_EQ_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_EQ(opaque_context, true, false);
    return kTfLiteOk;
  };
  EnsureDelegationFails();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*true != false.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_EQ_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_EQ(opaque_context, true, true);
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.* != *");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_MSG_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_MSG(opaque_context, false, "custom error msg");
    return kTfLiteOk;
  };
  EnsureDelegationFails();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*custom error msg.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_MSG_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_MSG(opaque_context, true, "custom error msg");
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_TYPES_EQ_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_TYPES_EQ(opaque_context, kTfLiteFloat32,
                                   kTfLiteInt32);
    return kTfLiteOk;
  };
  EnsureDelegationFails();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*kTfLiteFloat32 != kTfLiteInt32.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_TYPES_EQ_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_TYPES_EQ(opaque_context, kTfLiteFloat32,
                                   kTfLiteFloat32);
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*!=.*");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_NEAR_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_NEAR(opaque_context, 1, 10, 5);
    return kTfLiteOk;
  };
  EnsureDelegationFails();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*1 not near 10.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_NEAR_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_NEAR(opaque_context, 10, 10, 5);
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*tensorflow/lite/delegates/"
      "opaque_delegate_test\\.cc.*10 not near 10.*");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_MAYBE_KERNEL_LOG_REPORTS) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(
        opaque_context, "Report through TF_LITE_OPAQUE_MAYBE_KERNEL_LOG");
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*Report through TF_LITE_OPAQUE_MAYBE_KERNEL_LOG.*");
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  EXPECT_THAT(reporter_.error_messages(), ContainsRegex(txt_regex));
#else
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
#endif
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_MAYBE_KERNEL_LOG_SILENT) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_MAYBE_KERNEL_LOG(nullptr, "Should not be printed.");
    return kTfLiteOk;
  };
  EnsureDelegationSucceeds();
  const std::string txt_regex(
      ".*Report through TF_LITE_OPAQUE_MAYBE_KERNEL_LOG.*");
  EXPECT_THAT(reporter_.error_messages(), Not(ContainsRegex(txt_regex)));
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_MSG_EmptyString) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_MSG(opaque_context, false, "");
    return kTfLiteOk;
  };
  EnsureDelegationFails();
}
TEST_F(TestOpaqueMacros, TF_LITE_OPAQUE_ENSURE_MSG_WithFormattingChars) {
  opaque_delegate_builder_.Prepare = [](TfLiteOpaqueContext* opaque_context,
                                        TfLiteOpaqueDelegate* opaque_delegate,
                                        void* data) -> TfLiteStatus {
    TF_LITE_OPAQUE_ENSURE_MSG(opaque_context, false, "%i %d");
    return kTfLiteOk;
  };
  EnsureDelegationFails();
}
}  // anonymous namespace
}  // namespace delegates
}  // namespace tflite
