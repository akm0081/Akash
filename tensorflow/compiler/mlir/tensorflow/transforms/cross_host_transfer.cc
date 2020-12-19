/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

// This pass inserts tf_device.send and tf_device.receive ops to make sure any
// argument of any op is on the same host of the op itself.

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "mlir/IR/Attributes.h"  // from @llvm-project
#include "mlir/IR/Builders.h"  // from @llvm-project
#include "mlir/IR/Types.h"  // from @llvm-project
#include "mlir/Pass/PassManager.h"  // from @llvm-project
#include "mlir/Transforms/Passes.h"  // from @llvm-project
#include "tensorflow/compiler/mlir/tensorflow/ir/tf_device.h"
#include "tensorflow/core/util/device_name_utils.h"

namespace mlir {
namespace TF {

namespace {

using DeviceNameUtils = ::tensorflow::DeviceNameUtils;

constexpr const char *kOpDeviceAttr = "device";
constexpr const char *kArgDeviceAttr = "tf.device";
// TODO(b/175480458): Do not assign default host once every op in the TF
// dialect has the device attribute.
constexpr const char *kDefaultHost = "/job:localhost/replica:0/task:0";
constexpr const char *kCPUDevice = "/device:CPU:0";

// Return the job/replica/task from the device name as the host address. If no
// job/replica/task is specified, return /job:localhost/replica:0/task:0 as the
// default host address.
static std::string GetHost(const std::string &device) {
  DeviceNameUtils::ParsedName parsed_name;
  DeviceNameUtils::ParseFullName(device, &parsed_name);
  parsed_name.has_id = false;
  parsed_name.has_type = false;

  auto host = DeviceNameUtils::ParsedNameToString(parsed_name);
  if (host.empty()) return kDefaultHost;

  return host;
}

// Return a globally unique string as the rendezvous key for cross-host value
// transfer.
static std::string GetNextKey() {
  static int64_t next_index = 0;
  std::string key;
  llvm::raw_string_ostream os(key);
  os << "key-" << next_index;
  next_index++;

  return key;
}

struct CrossHostTransferPass
    : public PassWrapper<CrossHostTransferPass, FunctionPass> {
  void runOnFunction() override;
};

void CrossHostTransferPass::runOnFunction() {
  FuncOp func_op = getOperation();
  // This map is used to avoid transferring the same value to the same host
  // multiple times.
  llvm::DenseMap<mlir::Value, llvm::StringMap<mlir::Value>>
      transferred_value_by_value_and_host;

  func_op.getBody().walk([&](Operation *op) {
    if (op->isKnownTerminator()) return WalkResult::advance();

    OpBuilder builder(op);
    // Get the host address of the op.
    std::string op_device = "";
    if (StringAttr device_attr = op->getAttrOfType<StringAttr>(kOpDeviceAttr)) {
      op_device = device_attr.getValue().str();
    }
    std::string dst_host = GetHost(op_device);

    for (mlir::Value arg : op->getOperands()) {
      // Get the host address of the argument.
      std::string arg_device = "";
      if (BlockArgument block_arg = arg.dyn_cast<BlockArgument>()) {
        // Do not send this argument if it is not a function's argument. This
        // can happen when the argument is a while loop's argument.
        if (block_arg.getParentRegion() != &func_op.getRegion()) continue;

        if (StringAttr device_attr = func_op.getArgAttrOfType<StringAttr>(
                block_arg.getArgNumber(), kArgDeviceAttr)) {
          arg_device = device_attr.getValue().str();
        }
      } else {
        Operation *defining_op = arg.getDefiningOp();
        if (StringAttr device_attr =
                defining_op->getAttrOfType<StringAttr>(kOpDeviceAttr)) {
          arg_device = device_attr.getValue().str();
        }
      }
      std::string src_host = GetHost(arg_device);

      if (src_host == dst_host) continue;

      // Re-use the transferred argument if the argument has already been
      // transferred to the given host.
      llvm::StringMap<mlir::Value> &transferred_value_by_host =
          transferred_value_by_value_and_host[arg];
      auto iter = transferred_value_by_host.find(dst_host);
      if (iter != transferred_value_by_host.end()) {
        op->replaceUsesOfWith(arg, iter->second);
        continue;
      }

      // Create tf_device.send and tf_device.receive ops to send the argument to
      // the same host of the operation.
      std::string key = GetNextKey();
      auto send_op =
          builder.create<tf_device::SendOp>(op->getLoc(), arg, key, dst_host);
      send_op->setAttr(kOpDeviceAttr,
                       builder.getStringAttr(src_host + kCPUDevice));

      auto receive_op = builder.create<tf_device::ReceiveOp>(
          op->getLoc(), arg.getType(), key, src_host);
      receive_op->setAttr(kOpDeviceAttr,
                          builder.getStringAttr(dst_host + kCPUDevice));

      transferred_value_by_host[dst_host] = receive_op.getResult();
      op->replaceUsesOfWith(arg, receive_op.getResult());
    }
    return WalkResult::advance();
  });
}

}  // namespace

std::unique_ptr<FunctionPass> CreateCrossHostTransferPass() {
  return std::make_unique<CrossHostTransferPass>();
}

static PassRegistration<CrossHostTransferPass> pass(
    "tf-cross-host-transfer",
    "This pass inserts tf_device.send and tf_device.receive ops to make sure "
    "any argument of any op is on the same host of the op itself.");

}  // namespace TF
}  // namespace mlir
