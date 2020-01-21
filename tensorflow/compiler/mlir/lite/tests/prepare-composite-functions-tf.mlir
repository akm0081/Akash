// RUN: tf-opt -tfl-prepare-composite-funcs-tf %s -split-input-file | FileCheck %s --dump-input-on-failure

module{
func @embedding(%arg0: tensor<*xf32>, %arg1: tensor<*xi32>) -> tensor<*xf32> attributes  {tf._implements = "embedding_matmul", tf._reference = "mlir"} {
  %0 = "tf.Const"() {value = dense<1> : tensor<i32>} : () -> tensor<i32>
  %1 = "tf.ExpandDims"(%arg1, %0) : (tensor<*xi32>, tensor<i32>) -> tensor<*xi32>
  %2 = "tf.Const"() {value = dense<1> : tensor<i32>} : () -> tensor<i32>
  %3 = "tf.Const"() {value = dense<4096> : tensor<i32>} : () -> tensor<i32>
  %4 = "tf.Const"() {value = dense<0> : tensor<i32>} : () -> tensor<i32>
  %5 = "tf.Range"(%4, %3, %2) : (tensor<i32>, tensor<i32>, tensor<i32>) -> tensor<4096xi32>
  %6 = "tf.Equal"(%1, %5) : (tensor<*xi32>, tensor<4096xi32>) -> tensor<*xi1>
  %7 = "tf.Cast"(%6) : (tensor<*xi1>) -> tensor<*xf32>
  %8 = "tf.BatchMatMulV2"(%7, %arg0) {adj_x = false, adj_y = false} : (tensor<*xf32>, tensor<*xf32>) -> tensor<*xf32>
  return %8 : tensor<*xf32>
}

func @lstmcellsimple(%arg0: tensor<1x?xf32>, %arg1: tensor<3x4xf32>, %arg2: tensor<2xf32>, %arg3: tensor<1x3xf32>, %arg4: tensor<?xf32>) -> tensor<1x?xf32> attributes  {tf._implements = "LSTMCellSimple", tf._reference = "mlir"} {
    %0 = "tf.BatchMatMulV2"(%arg3, %arg1) {adj_x = false, adj_y = false} : (tensor<1x3xf32>, tensor<3x4xf32>) -> tensor<1x4xf32>
    %1 = constant dense<[[2.3, 3.4, 4.5, 5.5]]> : tensor<1x4xf32>
    %2 = "tf.Add"(%0, %1) : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %3 = tensor_cast %2 : tensor<1x4xf32> to tensor<1x?xf32>
    return %3 : tensor<1x?xf32>
}

func @layernormalizedlstmcellsimple(%arg0: tensor<1x?xf32>, %arg1: tensor<3x4xf32>, %arg2: tensor<2xf32>, %arg3: tensor<1x3xf32>, %arg4: tensor<2xf32>) -> tensor<1x?xf32> attributes  {tf._implements = "LayerNormalizedLstmCellSimple", tf._reference = "mlir"} {
    %0 = "tf.BatchMatMulV2"(%arg3, %arg1) {adj_x = false, adj_y = false} : (tensor<1x3xf32>, tensor<3x4xf32>) -> tensor<1x4xf32>
    %1 = constant dense<[[2.3, 3.4, 4.5, 5.5]]> : tensor<1x4xf32>
    %2 = "tf.Add"(%0, %1) : (tensor<1x4xf32>, tensor<1x4xf32>) -> tensor<1x4xf32>
    %3 = tensor_cast %2 : tensor<1x4xf32> to tensor<1x?xf32>
    return %3 : tensor<1x?xf32>
}

// NOTE: Assertions have been autogenerated by utils/generate-test-checks.py
// CHECK-LABEL:   func @embedding(
// CHECK-SAME:              [[VAL_0:%.*]]: tensor<*xf32>, [[VAL_1:%.*]]: tensor<*xi32>) -> tensor<*xf32>

// CHECK-LABEL:   attributes  {tf._implements = "embedding_lookup", tf._reference = "mlir"} {
// CHECK:           [[VAL_2:%.*]] = "tfl.embedding_lookup"([[VAL_1]], [[VAL_0]]) : (tensor<*xi32>, tensor<*xf32>) -> tensor<*xf32>
// CHECK:           return [[VAL_2]] : tensor<*xf32>

// CHECK-LABEL:   func @lstmcellsimple(
// CHECK-SAME:                          [[VAL_0]]: tensor<1x?xf32>, [[VAL_1]]: tensor<3x4xf32>, [[VAL_3:%.*]]: tensor<2xf32>, [[VAL_4:%.*]]: tensor<1x3xf32>, [[VAL_5:%.*]]: tensor<?xf32>) -> tensor<1x?xf32>

// CHECK-LABEL:   attributes  {tf._implements = "LSTMCellSimple", tf._reference = "mlir"} {
// CHECK:           [[VAL_6:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_7:%.*]] = "tf.Transpose"([[VAL_1]], [[VAL_6]]) : (tensor<3x4xf32>, tensor<2xi64>) -> tensor<4x3xf32>
// CHECK:           [[VAL_8:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_9:%.*]] = "tf.Transpose"([[VAL_4]], [[VAL_8]]) : (tensor<1x3xf32>, tensor<2xi64>) -> tensor<3x1xf32>
// CHECK:           [[VAL_10:%.*]] = constant unit
// CHECK:           [[VAL_11:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_12:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_13:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_11]], [[VAL_12]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_14:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_15:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_16:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_14]], [[VAL_15]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_17:%.*]] = constant dense<[2, 0]> : tensor<2xi64>
// CHECK:           [[VAL_18:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_19:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_17]], [[VAL_18]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_20:%.*]] = constant dense<[3, 0]> : tensor<2xi64>
// CHECK:           [[VAL_21:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_22:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_20]], [[VAL_21]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_23:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_24:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_25:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_23]], [[VAL_24]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_26:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_27:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_28:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_26]], [[VAL_27]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_29:%.*]] = constant dense<[2, 0]> : tensor<2xi64>
// CHECK:           [[VAL_30:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_31:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_29]], [[VAL_30]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_32:%.*]] = constant dense<[3, 0]> : tensor<2xi64>
// CHECK:           [[VAL_33:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_34:%.*]] = "tf.Slice"([[VAL_7]], [[VAL_32]], [[VAL_33]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_35:%.*]] = constant dense<0> : tensor<1xi64>
// CHECK:           [[VAL_36:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_37:%.*]] = "tf.Slice"([[VAL_3]], [[VAL_35]], [[VAL_36]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_38:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_39:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_40:%.*]] = "tf.Slice"([[VAL_3]], [[VAL_38]], [[VAL_39]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_41:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_42:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_43:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_44:%.*]] = constant dense<[3, 1]> : tensor<2xi64>
// CHECK:           [[VAL_45:%.*]] = "tf.Slice"([[VAL_9]], [[VAL_43]], [[VAL_44]]) : (tensor<3x1xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<3x1xf32>
// CHECK:           [[VAL_46:%.*]] = constant dense<0.000000e+00> : tensor<3xf32>
// CHECK:           [[VAL_47:%.*]] = constant dense<0.000000e+00> : tensor<1x3xf32>
// CHECK:           [[VAL_48:%.*]] = constant dense<0.000000e+00> : tensor<1x1xf32>
// CHECK:           [[VAL_49:%.*]] = "tfl.lstm"([[VAL_0]], [[VAL_16]], [[VAL_19]], [[VAL_13]], [[VAL_22]], [[VAL_28]], [[VAL_31]], [[VAL_25]], [[VAL_34]], [[VAL_10]], [[VAL_10]], [[VAL_10]], [[VAL_40]], [[VAL_41]], [[VAL_37]], [[VAL_42]], [[VAL_45]], [[VAL_46]], [[VAL_47]], [[VAL_48]], [[VAL_10]], [[VAL_10]], [[VAL_10]], [[VAL_10]]) ( {
// CHECK:           }) {cell_clip = 1.000000e+01 : f32, fused_activation_function = "TANH", kernel_type = "FULL", proj_clip = 0.000000e+00 : f32} : (tensor<1x?xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x3xf32>, tensor<1x3xf32>, tensor<1x3xf32>, tensor<1x3xf32>, none, none, none, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<3x1xf32>, tensor<3xf32>, tensor<1x3xf32>, tensor<1x1xf32>, none, none, none, none) -> tensor<1x3xf32>
// CHECK:           [[VAL_50:%.*]] = tensor_cast [[VAL_51:%.*]] : tensor<1x3xf32> to tensor<1x?xf32>
// CHECK:           return [[VAL_50]] : tensor<1x?xf32>

// CHECK-LABEL:   func @layernormalizedlstmcellsimple(
// CHECK-SAME:                                        [[VAL_0]]: tensor<1x?xf32>, [[VAL_1]]: tensor<3x4xf32>, [[VAL_3]]: tensor<2xf32>, [[VAL_4]]: tensor<1x3xf32>, [[VAL_5]]: tensor<2xf32>) -> tensor<1x?xf32>

// CHECK-LABEL:   attributes  {tf._implements = "LayerNormalizedLstmCellSimple", tf._reference = "mlir"} {
// CHECK:           [[VAL_52:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_53:%.*]] = "tf.Transpose"([[VAL_1]], [[VAL_52]]) : (tensor<3x4xf32>, tensor<2xi64>) -> tensor<4x3xf32>
// CHECK:           [[VAL_54:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_55:%.*]] = "tf.Transpose"([[VAL_4]], [[VAL_54]]) : (tensor<1x3xf32>, tensor<2xi64>) -> tensor<3x1xf32>
// CHECK:           [[VAL_56:%.*]] = constant unit
// CHECK:           [[VAL_57:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_58:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_59:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_57]], [[VAL_58]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_60:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_61:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_62:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_60]], [[VAL_61]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_63:%.*]] = constant dense<[2, 0]> : tensor<2xi64>
// CHECK:           [[VAL_64:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_65:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_63]], [[VAL_64]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_66:%.*]] = constant dense<[3, 0]> : tensor<2xi64>
// CHECK:           [[VAL_67:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_68:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_66]], [[VAL_67]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x0xf32>
// CHECK:           [[VAL_69:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_70:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_71:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_69]], [[VAL_70]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_72:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_73:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_74:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_72]], [[VAL_73]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_75:%.*]] = constant dense<[2, 0]> : tensor<2xi64>
// CHECK:           [[VAL_76:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_77:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_75]], [[VAL_76]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_78:%.*]] = constant dense<[3, 0]> : tensor<2xi64>
// CHECK:           [[VAL_79:%.*]] = constant dense<[1, 3]> : tensor<2xi64>
// CHECK:           [[VAL_80:%.*]] = "tf.Slice"([[VAL_53]], [[VAL_78]], [[VAL_79]]) : (tensor<4x3xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<1x3xf32>
// CHECK:           [[VAL_81:%.*]] = constant dense<0> : tensor<1xi64>
// CHECK:           [[VAL_82:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_83:%.*]] = "tf.Slice"([[VAL_3]], [[VAL_81]], [[VAL_82]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_84:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_85:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_86:%.*]] = "tf.Slice"([[VAL_3]], [[VAL_84]], [[VAL_85]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_87:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_88:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_89:%.*]] = constant dense<0> : tensor<2xi64>
// CHECK:           [[VAL_90:%.*]] = constant dense<[3, 1]> : tensor<2xi64>
// CHECK:           [[VAL_91:%.*]] = "tf.Slice"([[VAL_55]], [[VAL_89]], [[VAL_90]]) : (tensor<3x1xf32>, tensor<2xi64>, tensor<2xi64>) -> tensor<3x1xf32>
// CHECK:           [[VAL_92:%.*]] = constant dense<0.000000e+00> : tensor<3xf32>
// CHECK:           [[VAL_93:%.*]] = constant dense<0.000000e+00> : tensor<1x3xf32>
// CHECK:           [[VAL_94:%.*]] = constant dense<0.000000e+00> : tensor<1x1xf32>
// CHECK:           [[VAL_95:%.*]] = constant dense<0> : tensor<1xi64>
// CHECK:           [[VAL_96:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_97:%.*]] = "tf.Slice"([[VAL_5]], [[VAL_95]], [[VAL_96]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_98:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_99:%.*]] = constant dense<1> : tensor<1xi64>
// CHECK:           [[VAL_100:%.*]] = "tf.Slice"([[VAL_5]], [[VAL_98]], [[VAL_99]]) : (tensor<2xf32>, tensor<1xi64>, tensor<1xi64>) -> tensor<1xf32>
// CHECK:           [[VAL_101:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_102:%.*]] = constant dense<0.000000e+00> : tensor<1xf32>
// CHECK:           [[VAL_103:%.*]] = "tfl.lstm"([[VAL_0]], [[VAL_62]], [[VAL_65]], [[VAL_59]], [[VAL_68]], [[VAL_74]], [[VAL_77]], [[VAL_71]], [[VAL_80]], [[VAL_56]], [[VAL_56]], [[VAL_56]], [[VAL_86]], [[VAL_87]], [[VAL_83]], [[VAL_88]], [[VAL_91]], [[VAL_92]], [[VAL_93]], [[VAL_94]], [[VAL_100]], [[VAL_101]], [[VAL_97]], [[VAL_102]]) ( {
// CHECK:           }) {cell_clip = 1.000000e+01 : f32, fused_activation_function = "TANH", kernel_type = "FULL", proj_clip = 0.000000e+00 : f32} : (tensor<1x?xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x0xf32>, tensor<1x3xf32>, tensor<1x3xf32>, tensor<1x3xf32>, tensor<1x3xf32>, none, none, none, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<3x1xf32>, tensor<3xf32>, tensor<1x3xf32>, tensor<1x1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1x3xf32>
// CHECK:           [[VAL_104:%.*]] = tensor_cast [[VAL_105:%.*]] : tensor<1x3xf32> to tensor<1x?xf32>
// CHECK:           return [[VAL_104]] : tensor<1x?xf32>
}

// -----

module {
func @inference_standard_lstm_7410(%arg0: tensor<?x8x8xf32>, %arg1: tensor<?x10xf32>, %arg2: tensor<?x10xf32>, %arg3: tensor<8x40xf32>, %arg4: tensor<10x40xf32>, %arg5: tensor<40xf32>) -> (tensor<?x10xf32>, tensor<?x?x10xf32>, tensor<?x10xf32>, tensor<?x10xf32>, tensor<f32>) attributes {tf._input_shapes = ["tfshape$dim { size: -1 } dim { size: 8 } dim { size: 8 }", "tfshape$dim { size: -1 } dim { size: 10 }", "tfshape$dim { size: -1 } dim { size: 10 }", "tfshape$unknown_rank: true", "tfshape$unknown_rank: true", "tfshape$unknown_rank: true"], tf.api_implements = "lstm_b4e9f0e7-ac55-42bc-8ef2-8496419a608c", tf.api_preferred_device = "CPU", tf.signature.is_stateful} {
  %0 = "tf.BatchMatMulV2"(%arg0, %arg3) {adj_x = false, adj_y = false} : (tensor<?x8x8xf32>, tensor<8x40xf32>) -> tensor<?x8x40xf32>
  %1 = "tf.Add"(%0, %arg5) : (tensor<?x8x40xf32>, tensor<40xf32>) -> tensor<?x8x40xf32>
  %2 = "tf.BatchMatMulV2"(%1, %arg4) {adj_x = false, adj_y = true} : (tensor<?x8x40xf32>, tensor<10x40xf32>) -> tensor<?x8x10xf32>
  %3 = "tf.Add"(%2, %arg1) : (tensor<?x8x10xf32>, tensor<?x10xf32>) -> tensor<?x8x10xf32>
  %4 = "tf.Add"(%2, %arg2) : (tensor<?x8x10xf32>, tensor<?x10xf32>) -> tensor<?x?x10xf32>
  %5 = "tf.Add"(%arg1, %arg2) : (tensor<?x10xf32>, tensor<?x10xf32>) -> tensor<?x10xf32>
  %6 = "tf.Const"() {_output_shapes = ["tfshape$"], device = "/device:CPU:0", dtype = f32, value = dense<1.000000e+00> : tensor<f32>} : () -> tensor<f32>
  return %5, %4, %5, %5, %6 : tensor<?x10xf32>, tensor<?x?x10xf32>, tensor<?x10xf32>, tensor<?x10xf32>, tensor<f32>
}

// CHECK:       func @inference_standard_lstm_7410([[VAL_0:%.*]]: tensor<?x8x8xf32>, [[VAL_1:%.*]]: tensor<?x10xf32>, [[VAL_2:%.*]]: tensor<?x10xf32>, [[VAL_3:%.*]]: tensor<8x40xf32>, [[VAL_4:%.*]]: tensor<10x40xf32>, [[VAL_5:%.*]]: tensor<40xf32>) -> tensor<?x8x10xf32> attributes {tf._input_shapes = ["tfshape$dim { size: -1 } dim { size: 8 } dim { size: 8 }", "tfshape$dim { size: -1 } dim { size: 10 }", "tfshape$dim { size: -1 } dim { size: 10 }", "tfshape$unknown_rank: true", "tfshape$unknown_rank: true", "tfshape$unknown_rank: true"], tf.api_implements = "lstm_b4e9f0e7-ac55-42bc-8ef2-8496419a608c", tf.api_preferred_device = "CPU", tf.signature.is_stateful} {
// CHECK:           [[VAL_6:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_7:%.*]] = "tf.Transpose"([[VAL_3]], [[VAL_6]]) : (tensor<8x40xf32>, tensor<2xi64>) -> tensor<40x8xf32>
// CHECK:           [[VAL_8:%.*]] = constant dense<[1, 0]> : tensor<2xi64>
// CHECK:           [[VAL_9:%.*]] = "tf.Transpose"([[VAL_4]], [[VAL_8]]) : (tensor<10x40xf32>, tensor<2xi64>) -> tensor<40x10xf32>
// CHECK:           [[VAL_10:%.*]] = "tf.Const"() {value = dense<10> : tensor<4xi32>} : () -> tensor<4xi32>
// CHECK:           [[VAL_11:%.*]] = "tf.Const"() {value = dense<0> : tensor<i32>} : () -> tensor<i32>
// CHECK:           [[VAL_12:%.*]]:4 = "tf.SplitV"([[VAL_7]], [[VAL_10]], [[VAL_11]]) : (tensor<40x8xf32>, tensor<4xi32>, tensor<i32>) -> (tensor<10x8xf32>, tensor<10x8xf32>, tensor<10x8xf32>, tensor<10x8xf32>)
// CHECK:           [[VAL_13:%.*]] = "tf.Const"() {value = dense<10> : tensor<4xi32>} : () -> tensor<4xi32>
// CHECK:           [[VAL_14:%.*]] = "tf.Const"() {value = dense<0> : tensor<i32>} : () -> tensor<i32>
// CHECK:           [[VAL_15:%.*]]:4 = "tf.SplitV"([[VAL_9]], [[VAL_13]], [[VAL_14]]) : (tensor<40x10xf32>, tensor<4xi32>, tensor<i32>) -> (tensor<10x10xf32>, tensor<10x10xf32>, tensor<10x10xf32>, tensor<10x10xf32>)
// CHECK:           [[VAL_16:%.*]] = "tf.Const"() {value = dense<10> : tensor<4xi32>} : () -> tensor<4xi32>
// CHECK:           [[VAL_17:%.*]] = "tf.Const"() {value = dense<0> : tensor<i32>} : () -> tensor<i32>
// CHECK:           [[VAL_18:%.*]]:4 = "tf.SplitV"([[VAL_5]], [[VAL_16]], [[VAL_17]]) : (tensor<40xf32>, tensor<4xi32>, tensor<i32>) -> (tensor<10xf32>, tensor<10xf32>, tensor<10xf32>, tensor<10xf32>)
// CHECK:           [[VAL_19:%.*]] = constant unit
// CHECK:           [[VAL_20:%.*]] = "tfl.lstm"([[VAL_0]], [[VAL_12]]#0, [[VAL_12]]#1, [[VAL_12]]#2, [[VAL_12]]#3, [[VAL_15]]#0, [[VAL_15]]#1, [[VAL_15]]#2, [[VAL_15]]#3, [[VAL_19]], [[VAL_19]], [[VAL_19]], [[VAL_18]]#0, [[VAL_18]]#1, [[VAL_18]]#2, [[VAL_18]]#3, [[VAL_19]], [[VAL_19]], [[VAL_1]], [[VAL_2]], [[VAL_19]], [[VAL_19]], [[VAL_19]], [[VAL_19]]) ( {
// CHECK:           }) {cell_clip = 1.000000e+01 : f32, fused_activation_function = "TANH", kernel_type = "FULL", proj_clip = 0.000000e+00 : f32} : (tensor<?x8x8xf32>, tensor<10x8xf32>, tensor<10x8xf32>, tensor<10x8xf32>, tensor<10x8xf32>, tensor<10x10xf32>, tensor<10x10xf32>, tensor<10x10xf32>, tensor<10x10xf32>, none, none, none, tensor<10xf32>, tensor<10xf32>, tensor<10xf32>, tensor<10xf32>, none, none, tensor<?x10xf32>, tensor<?x10xf32>, none, none, none, none) -> tensor<?x8x10xf32>
// CHECK:           return [[VAL_21:%.*]] : tensor<?x8x10xf32>

}
