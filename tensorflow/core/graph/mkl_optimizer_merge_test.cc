/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

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

#ifdef INTEL_MKL

#include "tensorflow/core/graph/mkl_optimizer_merge.h"
#include <algorithm>
#include <vector>
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/graph_constructor.h"
#include "tensorflow/core/graph/testlib.h"
#include "tensorflow/core/kernels/ops_util.h"
#include "tensorflow/core/lib/random/simple_philox.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/test_benchmark.h"

namespace tensorflow {
namespace {

static void InitGraph(const string& s, Graph* graph) {
  GraphDef graph_def;

  auto parser = protobuf::TextFormat::Parser();
  //  parser.AllowRelaxedWhitespace(true);
  CHECK(parser.MergeFromString(s, &graph_def)) << s;
  GraphConstructorOptions opts;
  TF_CHECK_OK(ConvertGraphDefToGraph(opts, graph_def, graph));
}

class OptimizerMergeTest : public ::testing::Test {
 public:
  OptimizerMergeTest() : graph_(OpRegistry::Global()) {}

  void InitGraph(const string& s) {
    ::tensorflow::InitGraph(s, &graph_);
    original_ = CanonicalGraphString(&graph_);
  }

  static bool IncludeNode(const Node* n) { return n->IsOp(); }

  static string EdgeId(const Node* n, int index) {
    if (index == 0) {
      return n->name();
    } else if (index == Graph::kControlSlot) {
      return strings::StrCat(n->name(), ":control");
    } else {
      return strings::StrCat(n->name(), ":", index);
    }
  }

  string CanonicalGraphString(Graph* g) {
    std::vector<string> nodes;
    std::vector<string> edges;
    for (const Node* n : g->nodes()) {
      if (IncludeNode(n)) {
        nodes.push_back(strings::StrCat(n->name(), "(", n->type_string(), ")"));
      }
    }
    for (const Edge* e : g->edges()) {
      if (IncludeNode(e->src()) && IncludeNode(e->dst())) {
        edges.push_back(strings::StrCat(EdgeId(e->src(), e->src_output()), "->",
                                        EdgeId(e->dst(), e->dst_input())));
      }
    }
    // Canonicalize
    std::sort(nodes.begin(), nodes.end());
    std::sort(edges.begin(), edges.end());
    return strings::StrCat(str_util::Join(nodes, ";"), "|",
                           str_util::Join(edges, ";"));
  }

  string DoNodeMerge() {
    string before = CanonicalGraphString(&graph_);
    LOG(ERROR) << "Before node merge optimize: " << before;

    std::unique_ptr<Graph>* ug = new std::unique_ptr<Graph>(&graph_);
    OptimizeNodeMerge(ug);

    string result = CanonicalGraphString(&graph_);
    LOG(ERROR) << "After node merge optimize:  " << result;
    return result;
  }

  const string& OriginalGraph() const { return original_; }

  Graph graph_;
  string original_;
};

REGISTER_OP("Input").Output("o: float").SetIsStateful();

TEST_F(OptimizerMergeTest, Basic) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Mul' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Mul' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Mul);D(Mul)|"
            "A->C;A->D;B->C:1;B->D:1");
}

/* Test set 1: Conv2D + AddBias */

/*
 * C=Conv2D(A,B); E=BiasAdd(C,D); Z=Sub(E,Y)
 */
TEST_F(OptimizerMergeTest, Conv2DWithBias_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);D(Input);Y(Input);Z(Sub);n/_0(Conv2DWithBias)|"
             "A->n/_0;B->n/_0:1;D->n/_0:2;Y->Z:1;n/_0->Z");
  // Output is printed like this because nodes are sorted alphabetically.
}

/* Graph contains only Conv2D, no AddBias. */
TEST_F(OptimizerMergeTest, Conv2DWithBias_Negative_NoAddBias) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D)|"
             "A->C;B->C:1");
}

/* Conv2D output does not go to BiasAdd. */
TEST_F(OptimizerMergeTest, Conv2DWithBias_Negative_Dataflow1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }"); /* Output of Conv2D does not go to BiasAdd. */
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D);D(Input);E(Input);F(BiasAdd)|"
             "A->C;B->C:1;D->F;E->F:1");
}

/* Conv2D has two outgoing edges: BiasAdd and some other dummy node (Add).
   Merge should not be done in such case. */
TEST_F(OptimizerMergeTest, Conv2DWithBias_Negative_Dataflow2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }"  // Conv2D has two outputs.
                              // No merge should happen.
      "node { name: 'G' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D);D(Input);E(Input);F(BiasAdd);G(Add)|"
             "A->C;B->C:1;C->G;D->F;E->F:1;E->G:1");
}

/* data_format attribute value mismatch. Merge should not be done
   in such case. */
TEST_F(OptimizerMergeTest, Conv2DWithBias_Negative_AttrMismatch) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NHCW' } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D);D(Input);E(BiasAdd)|"
            "A->C;B->C:1;C->E;D->E:1");
}

#if 0
/* This test must fail because we have not used attributes. */
TEST_F(OptimizerMergeTest, Conv2DWithBias_NoAttrs) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd' input: ['A'] }");
  EXPECT_NE(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D);D(Mul);E(BiasAdd)|"
            "A->C;B->C:1;C->E;D->E:1");
}
#endif

/* Test set 2: MatMul + Add tests */

#if 0
/* Disabling these tests as we do not support merge of MatMul+Add now. */
/* C=MatMul(A,B); E=Add(C,D); Z=Sub(E, Y) */
TEST_F(OptimizerMergeTest, MatMulAdd_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Sub'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['E', 'Y'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);D(Input);Y(Input);Z(Sub);n/_0(MatMulMkl)|"
             "A->n/_0;B->n/_0:1;D->n/_0:2;Y->Z:1;n/_0->Z");
}

/* Check the case that output of MatMul may go to Shape node */
/* C=MatMul(A,B); E=Add(C,D); F=Shape(C); */
TEST_F(OptimizerMergeTest, MatMulAddShape_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }"
      "node { name: 'F' op: 'Shape'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);D(Input);F(Shape);n/_0(MatMulMkl)|"
             "A->n/_0;B->n/_0:1;D->n/_0:2;n/_0->F");
}

/* MatMul has 2 outputs: 1st output goes to Add, 2nd goes to Sub.*/
/* Merging will not happen in this case. */
/* C=MatMul(A,B); E=Add(C,D); F=Sub(C); */
TEST_F(OptimizerMergeTest, MatMulAddShape_Negative) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }"
      "node { name: 'F' op: 'Sub'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Input);E(Add);F(Sub)|"
             "A->C;B->C:1;C->E;C->F;D->E:1;D->F:1");
}

/* Only Matmul node, no Add node. Merge should not be done. */
TEST_F(OptimizerMergeTest, MatMulAdd_Negative_NoAdd) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul)|"
             "A->C;B->C:1");
}

/* MatMul output does not go to Add. Merge should not be done. */
TEST_F(OptimizerMergeTest, MatMulAdd_Negative_Dataflow1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['D', 'E'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Input);E(Input);F(Add)|"
             "A->C;B->C:1;D->F;E->F:1");
}

/* MatMul has two outgoing edges: Add and some other dummy node (Sub).
   Merge should not be done in such case. */
TEST_F(OptimizerMergeTest, MatMulAdd_Negative_Dataflow2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }"
      "node { name: 'F' op: 'Sub'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'D']}");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Input);E(Add);F(Sub)|"
             "A->C;B->C:1;C->E;C->F;D->E:1;D->F:1");
}
#endif

#if 0
/* Values of type (T) do not match. */
TEST_F(OptimizerMergeTest, MatMulAdd_Negative_Attrmismatch) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Add'"
      " attr { key: 'T'                value { type: DT_DOUBLE } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Input);E(Add)|"
             "A->C;B->C:1;C->E;D->E:1;");
}
#endif

/* Test set 3: Conv2D..BiasAddGrad -> Conv2DWithBiasBackpropBias
   rewrite tests */

/* C=Conv2D(A,B); D=Sub(C,A); F=BiasAddGrad(D) */
TEST_F(OptimizerMergeTest, Conv2DBackprop_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Conv2D);D(Sub);E(Conv2DWithBiasBackpropBias)|"
             "A->C;A->D:1;B->C:1;C->D;D->E");
}

/* No Conv2D in the context for BiasAddGrad. No rewrite should happen. */
/* C=Add(A,B); D=Sub(C,A); F=BiasAddGrad(D,E) */
TEST_F(OptimizerMergeTest, Conv2DBackprop_Negative_NoConv2D) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Add);D(Sub);E(BiasAddGrad)|"
             "A->C;A->D:1;B->C:1;C->D;D->E");
}

/* No Conv2D in the context for BiasAddGrad, but MatMul in context.
 Rewrite should happen, but name of BiasAddGrad does not change. */
/* C=MatMul(A,B); D=Sub(C,A); F=BiasAddGrad(D,E) */
TEST_F(OptimizerMergeTest, Conv2DBackprop_Negative_NoConv2D_MatMul) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Sub);E(BiasAddGrad)|"
             "A->C;A->D:1;B->C:1;C->D;D->E");
}

/* Test set 4: MatMulMkl..BiasAddGrad -> BiasAddGrad rewrite tests */

/* C=MatMul(A,B); D=Sub(C,A); F=BiasAddGrad(D,E) */
TEST_F(OptimizerMergeTest, MatMulBiasAddGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(MatMul);D(Sub);E(BiasAddGrad)|"
             "A->C;A->D:1;B->C:1;C->D;D->E");
      /* BiasAddGrad for MatMul is written into BiasAddGrad */
}

/* C=MatMulMkl(A,B,B1); D=Sub(C,A); F=BiasAddGrad(D,E) */
TEST_F(OptimizerMergeTest, MatMulMklBiasAddGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'MatMulMkl'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B', 'C']}"
      "node { name: 'E' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Input);D(MatMulMkl);E(Sub);F(BiasAddGrad)|"
             "A->D;A->E:1;B->D:1;C->D:2;D->E;E->F");
      /* BiasAddGrad for MatMul is written into BiasAddGrad */
}

/* No MatMul/MatMulMkl in the context for BiasAddGrad. No rewrite should
   happen. */
/* C=Add(A,B); D=Sub(C,A); F=BiasAddGrad(D,E) */
TEST_F(OptimizerMergeTest, MatMulBiasAddGrad_Negative_NoMatMul) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Add'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Sub'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoNodeMerge(),
            "A(Input);B(Input);C(Add);D(Sub);E(BiasAddGrad)|"
             "A->C;A->D:1;B->C:1;C->D;D->E");
}


static void BM_NodeMerge(int iters, int op_nodes) {
  testing::StopTiming();
  string s;
  for (int in = 0; in < 10; in++) {
    s += strings::Printf("node { name: 'in%04d' op: 'Input'}", in);
  }
  random::PhiloxRandom philox(301, 17);
  random::SimplePhilox rnd(&philox);
  for (int op = 0; op < op_nodes; op++) {
    s += strings::Printf(
        "node { name: 'op%04d' op: 'Mul' attr { key: 'T' value { "
        "type: DT_FLOAT } } input: ['in%04d', 'in%04d' ] }",
        op, rnd.Uniform(10), rnd.Uniform(10));
  }

  bool first = true;
  while (iters > 0) {
    Graph* graph = new Graph(OpRegistry::Global());
    InitGraph(s, graph);
    int N = graph->num_node_ids();
    if (first) {
      testing::SetLabel(strings::StrCat("Per graph node.  Nodes: ", N));
      first = false;
    }
    {
      testing::StartTiming();
      std::unique_ptr<Graph> ug(graph);
      OptimizeNodeMerge(&ug);
      testing::StopTiming();
    }
    iters -= N;  // Our benchmark units are individual graph nodes,
                 // not whole graphs
    // delete graph;
  }
}
BENCHMARK(BM_NodeMerge)->Arg(1000)->Arg(10000);

}  // namespace
}  // namespace tensorflow

#endif /* INTEL_MKL */
