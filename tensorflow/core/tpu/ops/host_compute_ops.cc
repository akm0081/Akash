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

#include "tensorflow/core/framework/attr_value.pb.h"
#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/shape_inference.h"

namespace tensorflow {

using shape_inference::InferenceContext;
using shape_inference::ShapeHandle;

REGISTER_OP("_HostComputeMlir")
    .Input("inputs: Tinputs")
    .Output("outputs: Toutputs")
    .Attr("Tinputs: list(type) >= 0")
    .Attr("Toutputs: list(type) >= 0")
    .Attr("key: string")
    .Attr("tpu_core: int = 0")
    .SetIsStateful()
    .Doc(R"doc(
A host-side computation called from a TPU device.

inputs: A list of tensors that will be sent to the host.
outputs: A list of tensors that will be returned to the device.
Tinputs: The element types of each element in `inputs`.
Toutputs: The element types of each element in `outputs`.
key: A unique identifier for this region used to match up host transfers.
tpu_core: Default core to use for host to device transfers.
)doc");

REGISTER_OP("XlaHostCompute")
    .Input("inputs: Tinputs")
    .Output("outputs: Toutputs")
    .Attr("Tinputs: list(type) >= 0")
    .Attr("Toutputs: list(type) >= 0")
    .Attr("ancestors: list(string) >= 0")
    .Attr("shapes: list(shape) >= 0")
    .Attr("shape_inference_graph: func")
    .Attr("key: string")
    .Attr("cost_estimate_ns: int=1000000")
    .Attr("tpu_core: int = 0")
    .SetIsStateful()
    .SetShapeFn([](shape_inference::InferenceContext* c) {
      const AttrValue* graph;
      TF_RETURN_IF_ERROR(c->attrs().Find("shape_inference_graph", &graph));
      if (graph->func().name().empty()) {
        const AttrValue* shapes;
        TF_RETURN_IF_ERROR(c->attrs().Find("shapes", &shapes));
        if (shapes->list().shape_size() != c->num_outputs()) {
          return errors::InvalidArgument(
              "_XlaHostCompute has ", c->num_outputs(),
              " outputs but 'shapes' attr has ", shapes->list().shape_size(),
              " elements");
        }
        for (int i = 0; i < c->num_outputs(); ++i) {
          shape_inference::ShapeHandle handle;
          TF_RETURN_IF_ERROR(
              c->MakeShapeFromShapeProto(shapes->list().shape(i), &handle));
          c->set_output(i, handle);
        }
        return Status::OK();
      } else {
        // There is a shape inference graph so the output shapes are not
        // statically known.
        return ::tensorflow::shape_inference::UnknownShape(c);
      }
    })
    .Doc(R"doc(
A pseudo-op to represent host-side computation in an XLA program.

inputs: A list of tensors that will be sent to the host.
outputs: A list of tensors that will be returned to the device.
Tinputs: The element types of each element in `inputs`.
Toutputs: The element types of each element in `outputs`.
ancestors: A list of names of HostCompute computations that must be
sequenced before this computation.
shape_inference_graph: If non-empty, a serialized GraphDef representing a graph
that must be analyzed at compile time to determine the shapes of the outputs.
shapes: If shape_inference_graph is empty, a list of the shapes of `outputs`.
key: A unique identifier for this region used to match up host transfers.
cost_estimate_ns: Estimated duration of the host computation in nanoseconds.
tpu_core: Default core to use for host to device transfers.
)doc");

REGISTER_OP("XlaSendToHost")
    .Input("input: Tinput")
    .Attr("Tinput: type")
    .Attr("key: string")
    .SetIsStateful()
    .Doc(R"doc(
An op to send a tensor to the host.

input: the tensor that will be sent to the host.
Tinput: element type for input.
key: A unique identifier for this region used to match up host transfers.
)doc");

REGISTER_OP("XlaRecvFromHost")
    .Output("output: Toutput")
    .Attr("Toutput: type")
    .Attr("shape: shape")
    .Attr("key: string")
    .SetIsStateful()
    .SetShapeFn([](shape_inference::InferenceContext* c) {
      const AttrValue* shape_attr;
      TF_RETURN_IF_ERROR(c->attrs().Find("shape", &shape_attr));
      if (!shape_attr->has_shape()) {
        return errors::InvalidArgument(
            "XlaRecvFromHost op does not have valid \"Toutput\" attr.");
      }
      shape_inference::ShapeHandle handle;
      TF_RETURN_IF_ERROR(
          c->MakeShapeFromShapeProto(shape_attr->shape(), &handle));
      c->set_output(0, handle);
      return Status::OK();
    })
    .Doc(R"doc(
An op to receive a tensor from the host.

output: the tensor that will be received from the host.
Toutput: element type for output.
shape: shape for output.
key: A unique identifier for this region used to match up host transfers.
)doc");

}  // namespace tensorflow
