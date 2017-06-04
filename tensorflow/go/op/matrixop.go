// Copyright 2017 The TensorFlow Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// DO NOT EDIT
// This file was machine generated by github.com/ctava/tensorflow/tensorflow/go/genop/wrap
//
// WARNING: This generation of wrapper function for TensorFlow ops is in an
// experimental state. The generated API can change without notice.

package op

import tf "github.com/tensorflow/tensorflow/tensorflow/go"

// Returns a batched matrix tensor with new batched diagonal values.
//
// Given `input` and `diagonal`, this operation returns a tensor with the
// same shape and values as `input`, except for the main diagonal of the
// innermost matrices.  These will be overwritten by the values in `diagonal`.
// 
// The output is computed as follows:
// 
// Assume `input` has `k+1` dimensions `[I, J, K, ..., M, N]` and `diagonal` has
// `k` dimensions `[I, J, K, ..., min(M, N)]`.  Then the output is a
// tensor of rank `k+1` with dimensions `[I, J, K, ..., M, N]` where:
// 
//   * `output[i, j, k, ..., m, n] = diagonal[i, j, k, ..., n]` for `m == n`.
//   * `output[i, j, k, ..., m, n] = input[i, j, k, ..., m, n]` for `m != n`.
//
// Arguments:
//	input: Rank `k+1`, where `k >= 1`.
//	diagonal: Rank `k`, where `k >= 1`.
//
// Returns Rank `k+1`, with `output.shape = input.shape`.
func MatrixSetDiag(scope *Scope, input tf.Output, diagonal tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixSetDiag",
		Input: []tf.Input{
			input, diagonal, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixSolveAttr is an optional argument to MatrixSolve.
type MatrixSolveAttr func(optionalAttr)


// MatrixSolveAdjoint sets the optional adjoint attribute to value.
//
// value: Boolean indicating whether to solve with `matrix` or its (block-wise)
// adjoint.
// If not specified, defaults to false 
func MatrixSolveAdjoint(value bool) MatrixSolveAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Solves systems of linear equations.
//
// `Matrix` is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. `Rhs` is a tensor of shape `[..., M, K]`. The `output` is
// a tensor shape `[..., M, K]`.  If `adjoint` is `False` then each output matrix
// satisfies `matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]`.
// If `adjoint` is `True` then each output matrix satisfies
// `adjoint(matrix[..., :, :]) * output[..., :, :] = rhs[..., :, :]`.
//
// Arguments:
//	matrix: Shape is `[..., M, M]`.
//	rhs: Shape is `[..., M, K]`.
//
// Returns Shape is `[..., M, K]`.
func MatrixSolve(scope *Scope, matrix tf.Output, rhs tf.Output, optional ...MatrixSolveAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixSolve",
		Input: []tf.Input{
			matrix, rhs, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixSolveLsAttr is an optional argument to MatrixSolveLs.
type MatrixSolveLsAttr func(optionalAttr)


// MatrixSolveLsFast sets the optional fast attribute to value.
// If not specified, defaults to true 
func MatrixSolveLsFast(value bool) MatrixSolveLsAttr {
	return func(m optionalAttr) {
		m["fast"] = value
	}
}

// Solves one or more linear least-squares problems.
//
// `matrix` is a tensor of shape `[..., M, N]` whose inner-most 2 dimensions
// form matrices of size `[M, N]`. Rhs is a tensor of shape `[..., M, K]`.
// The output is a tensor shape `[..., N, K]` where each output matrix solves
// each of the equations matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]
// in the least squares sense.
// 
// matrix and right-hand sides in the batch:
// 
// `matrix`=\\(A \in \Re^{m \times n}\\),
// `rhs`=\\(B  \in \Re^{m \times k}\\),
// `output`=\\(X  \in \Re^{n \times k}\\),
// `l2_regularizer`=\\(\lambda\\).
// 
// If `fast` is `True`, then the solution is computed by solving the normal
// equations using Cholesky decomposition. Specifically, if \\(m \ge n\\) then
// \\(X = (A^T A + \lambda I)^{-1} A^T B\\), which solves the least-squares
// problem \\(X = \mathrm{argmin}_{Z \in \Re^{n \times k} } ||A Z - B||_F^2 +
// \lambda ||Z||_F^2\\). If \\(m \lt n\\) then `output` is computed as
// \\(X = A^T (A A^T + \lambda I)^{-1} B\\), which (for \\(\lambda = 0\\)) is the
// minimum-norm solution to the under-determined linear system, i.e.
// \\(X = \mathrm{argmin}_{Z \in \Re^{n \times k} } ||Z||_F^2 \\), subject to
// \\(A Z = B\\). Notice that the fast path is only numerically stable when
// \\(A\\) is numerically full rank and has a condition number
// \\(\mathrm{cond}(A) \lt \frac{1}{\sqrt{\epsilon_{mach} } }\\) or\\(\lambda\\) is
// sufficiently large.
// 
// If `fast` is `False` an algorithm based on the numerically robust complete
// orthogonal decomposition is used. This computes the minimum-norm
// least-squares solution, even when \\(A\\) is rank deficient. This path is
// typically 6-7 times slower than the fast path. If `fast` is `False` then
// `l2_regularizer` is ignored.
//
// Arguments:
//	matrix: Shape is `[..., M, N]`.
//	rhs: Shape is `[..., M, K]`.
//	l2_regularizer: Scalar tensor.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.lstsq
// @end_compatibility
//
// Returns Shape is `[..., N, K]`.
func MatrixSolveLs(scope *Scope, matrix tf.Output, rhs tf.Output, l2_regularizer tf.Output, optional ...MatrixSolveLsAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixSolveLs",
		Input: []tf.Input{
			matrix, rhs, l2_regularizer, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixInverseAttr is an optional argument to MatrixInverse.
type MatrixInverseAttr func(optionalAttr)


// MatrixInverseAdjoint sets the optional adjoint attribute to value.
// If not specified, defaults to false 
func MatrixInverseAdjoint(value bool) MatrixInverseAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Computes the inverse of one or more square invertible matrices or their
//
// adjoints (conjugate transposes).
// 
// The input is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. The output is a tensor of the same shape as the input
// containing the inverse for all input submatrices `[..., :, :]`.
// 
// The op uses LU decomposition with partial pivoting to compute the inverses.
// 
// If a matrix is not invertible there is no guarantee what the op does. It
// may detect the condition and raise an exception or it may simply return a
// garbage result.
//
// Arguments:
//	input: Shape is `[..., M, M]`.
//
// Returns Shape is `[..., M, M]`.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.inv
// @end_compatibility
func MatrixInverse(scope *Scope, input tf.Output, optional ...MatrixInverseAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixInverse",
		Input: []tf.Input{
			input, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Copy a tensor setting everything outside a central band in each innermost matrix
//
// to zero.
// 
// The `band` part is computed as follows:
// Assume `input` has `k` dimensions `[I, J, K, ..., M, N]`, then the output is a
// tensor with the same shape where
// 
// `band[i, j, k, ..., m, n] = in_band(m, n) * input[i, j, k, ..., m, n]`.
// 
// The indicator function
// 
// `in_band(m, n) = (num_lower < 0 || (m-n) <= num_lower)) &&
//                  (num_upper < 0 || (n-m) <= num_upper)`.
// 
// For example:
// 
// ```prettyprint
// # if 'input' is [[ 0,  1,  2, 3]
//                  [-1,  0,  1, 2]
//                  [-2, -1,  0, 1]
//                  [-3, -2, -1, 0]],
// 
// tf.matrix_band_part(input, 1, -1) ==> [[ 0,  1,  2, 3]
//                                        [-1,  0,  1, 2]
//                                        [ 0, -1,  0, 1]
//                                        [ 0,  0, -1, 0]],
// 
// tf.matrix_band_part(input, 2, 1) ==> [[ 0,  1,  0, 0]
//                                       [-1,  0,  1, 0]
//                                       [-2, -1,  0, 1]
//                                       [ 0, -2, -1, 0]]
// ```
// 
// Useful special cases:
// 
// ```prettyprint
//  tf.matrix_band_part(input, 0, -1) ==> Upper triangular part.
//  tf.matrix_band_part(input, -1, 0) ==> Lower triangular part.
//  tf.matrix_band_part(input, 0, 0) ==> Diagonal.
// ```
//
// Arguments:
//	input: Rank `k` tensor.
//	num_lower: 0-D tensor. Number of subdiagonals to keep. If negative, keep entire
// lower triangle.
//	num_upper: 0-D tensor. Number of superdiagonals to keep. If negative, keep
// entire upper triangle.
//
// Returns Rank `k` tensor of the same shape as input. The extracted banded tensor.
func MatrixBandPart(scope *Scope, input tf.Output, num_lower tf.Output, num_upper tf.Output)(band tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixBandPart",
		Input: []tf.Input{
			input, num_lower, num_upper, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixTriangularSolveAttr is an optional argument to MatrixTriangularSolve.
type MatrixTriangularSolveAttr func(optionalAttr)


// MatrixTriangularSolveLower sets the optional lower attribute to value.
//
// value: Boolean indicating whether the innermost matrices in `matrix` are
// lower or upper triangular.
// If not specified, defaults to true 
func MatrixTriangularSolveLower(value bool) MatrixTriangularSolveAttr {
	return func(m optionalAttr) {
		m["lower"] = value
	}
}

// MatrixTriangularSolveAdjoint sets the optional adjoint attribute to value.
//
// value: Boolean indicating whether to solve with `matrix` or its (block-wise)
//          adjoint.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.triangular_solve
// @end_compatibility
// If not specified, defaults to false 
func MatrixTriangularSolveAdjoint(value bool) MatrixTriangularSolveAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Solves systems of linear equations with upper or lower triangular matrices by
//
// backsubstitution.
// 
// `matrix` is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions form
// square matrices. If `lower` is `True` then the strictly upper triangular part
// of each inner-most matrix is assumed to be zero and not accessed.
// If `lower` is False then the strictly lower triangular part of each inner-most
// matrix is assumed to be zero and not accessed.
// `rhs` is a tensor of shape `[..., M, K]`.
// 
// The output is a tensor of shape `[..., M, K]`. If `adjoint` is
// `True` then the innermost matrices in output` satisfy matrix equations
// `matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]`.
// If `adjoint` is `False` then the strictly then the  innermost matrices in
// `output` satisfy matrix equations
// `adjoint(matrix[..., i, k]) * output[..., k, j] = rhs[..., i, j]`.
//
// Arguments:
//	matrix: Shape is `[..., M, M]`.
//	rhs: Shape is `[..., M, K]`.
//
// Returns Shape is `[..., M, K]`.
func MatrixTriangularSolve(scope *Scope, matrix tf.Output, rhs tf.Output, optional ...MatrixTriangularSolveAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixTriangularSolve",
		Input: []tf.Input{
			matrix, rhs, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns the batched diagonal part of a batched tensor.
//
// This operation returns a tensor with the `diagonal` part
// of the batched `input`. The `diagonal` part is computed as follows:
// 
// Assume `input` has `k` dimensions `[I, J, K, ..., M, N]`, then the output is a
// tensor of rank `k - 1` with dimensions `[I, J, K, ..., min(M, N)]` where:
// 
// `diagonal[i, j, k, ..., n] = input[i, j, k, ..., n, n]`.
// 
// The input must be at least a matrix.
// 
// For example:
// 
// ```prettyprint
// # 'input' is [[[1, 0, 0, 0]
//                [0, 2, 0, 0]
//                [0, 0, 3, 0]
//                [0, 0, 0, 4]],
//               [[5, 0, 0, 0]
//                [0, 6, 0, 0]
//                [0, 0, 7, 0]
//                [0, 0, 0, 8]]]
// 
// and input.shape = (2, 4, 4)
// 
// tf.matrix_diag_part(input) ==> [[1, 2, 3, 4], [5, 6, 7, 8]]
// 
// which has shape (2, 4)
// ```
//
// Arguments:
//	input: Rank `k` tensor where `k >= 2`.
//
// Returns The extracted diagonal(s) having shape
// `diagonal.shape = input.shape[:-2] + [min(input.shape[-2:])]`.
func MatrixDiagPart(scope *Scope, input tf.Output)(diagonal tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDiagPart",
		Input: []tf.Input{
			input, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns a batched diagonal tensor with a given batched diagonal values.
//
// Given a `diagonal`, this operation returns a tensor with the `diagonal` and
// everything else padded with zeros. The diagonal is computed as follows:
// 
// Assume `diagonal` has `k` dimensions `[I, J, K, ..., N]`, then the output is a
// tensor of rank `k+1` with dimensions [I, J, K, ..., N, N]` where:
// 
// `output[i, j, k, ..., m, n] = 1{m=n} * diagonal[i, j, k, ..., n]`.
// 
// For example:
// 
// ```prettyprint
// # 'diagonal' is [[1, 2, 3, 4], [5, 6, 7, 8]]
// 
// and diagonal.shape = (2, 4)
// 
// tf.matrix_diag(diagonal) ==> [[[1, 0, 0, 0]
//                                      [0, 2, 0, 0]
//                                      [0, 0, 3, 0]
//                                      [0, 0, 0, 4]],
//                                     [[5, 0, 0, 0]
//                                      [0, 6, 0, 0]
//                                      [0, 0, 7, 0]
//                                      [0, 0, 0, 8]]]
// 
// which has shape (2, 4, 4)
// ```
//
// Arguments:
//	diagonal: Rank `k`, where `k >= 1`.
//
// Returns Rank `k+1`, with `output.shape = diagonal.shape + [diagonal.shape[-1]]`.
func MatrixDiag(scope *Scope, diagonal tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDiag",
		Input: []tf.Input{
			diagonal, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Computes the determinant of one ore more square matrices.
//
// The input is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. The output is a tensor containing the determinants
// for all input submatrices `[..., :, :]`.
//
// Arguments:
//	input: Shape is `[..., M, M]`.
//
// Returns Shape is `[...]`.
func MatrixDeterminant(scope *Scope, input tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDeterminant",
		Input: []tf.Input{
			input, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns a batched matrix tensor with new batched diagonal values.
//
// Given `input` and `diagonal`, this operation returns a tensor with the
// same shape and values as `input`, except for the main diagonal of the
// innermost matrices.  These will be overwritten by the values in `diagonal`.
// 
// The output is computed as follows:
// 
// Assume `input` has `k+1` dimensions `[I, J, K, ..., M, N]` and `diagonal` has
// `k` dimensions `[I, J, K, ..., min(M, N)]`.  Then the output is a
// tensor of rank `k+1` with dimensions `[I, J, K, ..., M, N]` where:
// 
//   * `output[i, j, k, ..., m, n] = diagonal[i, j, k, ..., n]` for `m == n`.
//   * `output[i, j, k, ..., m, n] = input[i, j, k, ..., m, n]` for `m != n`.
//
// Arguments:
//	input: Rank `k+1`, where `k >= 1`.
//	diagonal: Rank `k`, where `k >= 1`.
//
// Returns Rank `k+1`, with `output.shape = input.shape`.
func MatrixSetDiag(scope *Scope, input tf.Output, diagonal tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixSetDiag",
		Input: []tf.Input{
			input, diagonal, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixSolveAttr is an optional argument to MatrixSolve.
type MatrixSolveAttr func(optionalAttr)


// MatrixSolveAdjoint sets the optional adjoint attribute to value.
//
// value: Boolean indicating whether to solve with `matrix` or its (block-wise)
// adjoint.
// If not specified, defaults to false 
func MatrixSolveAdjoint(value bool) MatrixSolveAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Solves systems of linear equations.
//
// `Matrix` is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. `Rhs` is a tensor of shape `[..., M, K]`. The `output` is
// a tensor shape `[..., M, K]`.  If `adjoint` is `False` then each output matrix
// satisfies `matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]`.
// If `adjoint` is `True` then each output matrix satisfies
// `adjoint(matrix[..., :, :]) * output[..., :, :] = rhs[..., :, :]`.
//
// Arguments:
//	matrix: Shape is `[..., M, M]`.
//	rhs: Shape is `[..., M, K]`.
//
// Returns Shape is `[..., M, K]`.
func MatrixSolve(scope *Scope, matrix tf.Output, rhs tf.Output, optional ...MatrixSolveAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixSolve",
		Input: []tf.Input{
			matrix, rhs, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixSolveLsAttr is an optional argument to MatrixSolveLs.
type MatrixSolveLsAttr func(optionalAttr)


// MatrixSolveLsFast sets the optional fast attribute to value.
// If not specified, defaults to true 
func MatrixSolveLsFast(value bool) MatrixSolveLsAttr {
	return func(m optionalAttr) {
		m["fast"] = value
	}
}

// Solves one or more linear least-squares problems.
//
// `matrix` is a tensor of shape `[..., M, N]` whose inner-most 2 dimensions
// form matrices of size `[M, N]`. Rhs is a tensor of shape `[..., M, K]`.
// The output is a tensor shape `[..., N, K]` where each output matrix solves
// each of the equations matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]
// in the least squares sense.
// 
// matrix and right-hand sides in the batch:
// 
// `matrix`=\\(A \in \Re^{m \times n}\\),
// `rhs`=\\(B  \in \Re^{m \times k}\\),
// `output`=\\(X  \in \Re^{n \times k}\\),
// `l2_regularizer`=\\(\lambda\\).
// 
// If `fast` is `True`, then the solution is computed by solving the normal
// equations using Cholesky decomposition. Specifically, if \\(m \ge n\\) then
// \\(X = (A^T A + \lambda I)^{-1} A^T B\\), which solves the least-squares
// problem \\(X = \mathrm{argmin}_{Z \in \Re^{n \times k} } ||A Z - B||_F^2 +
// \lambda ||Z||_F^2\\). If \\(m \lt n\\) then `output` is computed as
// \\(X = A^T (A A^T + \lambda I)^{-1} B\\), which (for \\(\lambda = 0\\)) is the
// minimum-norm solution to the under-determined linear system, i.e.
// \\(X = \mathrm{argmin}_{Z \in \Re^{n \times k} } ||Z||_F^2 \\), subject to
// \\(A Z = B\\). Notice that the fast path is only numerically stable when
// \\(A\\) is numerically full rank and has a condition number
// \\(\mathrm{cond}(A) \lt \frac{1}{\sqrt{\epsilon_{mach} } }\\) or\\(\lambda\\) is
// sufficiently large.
// 
// If `fast` is `False` an algorithm based on the numerically robust complete
// orthogonal decomposition is used. This computes the minimum-norm
// least-squares solution, even when \\(A\\) is rank deficient. This path is
// typically 6-7 times slower than the fast path. If `fast` is `False` then
// `l2_regularizer` is ignored.
//
// Arguments:
//	matrix: Shape is `[..., M, N]`.
//	rhs: Shape is `[..., M, K]`.
//	l2_regularizer: Scalar tensor.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.lstsq
// @end_compatibility
//
// Returns Shape is `[..., N, K]`.
func MatrixSolveLs(scope *Scope, matrix tf.Output, rhs tf.Output, l2_regularizer tf.Output, optional ...MatrixSolveLsAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixSolveLs",
		Input: []tf.Input{
			matrix, rhs, l2_regularizer, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixInverseAttr is an optional argument to MatrixInverse.
type MatrixInverseAttr func(optionalAttr)


// MatrixInverseAdjoint sets the optional adjoint attribute to value.
// If not specified, defaults to false 
func MatrixInverseAdjoint(value bool) MatrixInverseAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Computes the inverse of one or more square invertible matrices or their
//
// adjoints (conjugate transposes).
// 
// The input is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. The output is a tensor of the same shape as the input
// containing the inverse for all input submatrices `[..., :, :]`.
// 
// The op uses LU decomposition with partial pivoting to compute the inverses.
// 
// If a matrix is not invertible there is no guarantee what the op does. It
// may detect the condition and raise an exception or it may simply return a
// garbage result.
//
// Arguments:
//	input: Shape is `[..., M, M]`.
//
// Returns Shape is `[..., M, M]`.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.inv
// @end_compatibility
func MatrixInverse(scope *Scope, input tf.Output, optional ...MatrixInverseAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixInverse",
		Input: []tf.Input{
			input, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Copy a tensor setting everything outside a central band in each innermost matrix
//
// to zero.
// 
// The `band` part is computed as follows:
// Assume `input` has `k` dimensions `[I, J, K, ..., M, N]`, then the output is a
// tensor with the same shape where
// 
// `band[i, j, k, ..., m, n] = in_band(m, n) * input[i, j, k, ..., m, n]`.
// 
// The indicator function
// 
// `in_band(m, n) = (num_lower < 0 || (m-n) <= num_lower)) &&
//                  (num_upper < 0 || (n-m) <= num_upper)`.
// 
// For example:
// 
// ```prettyprint
// # if 'input' is [[ 0,  1,  2, 3]
//                  [-1,  0,  1, 2]
//                  [-2, -1,  0, 1]
//                  [-3, -2, -1, 0]],
// 
// tf.matrix_band_part(input, 1, -1) ==> [[ 0,  1,  2, 3]
//                                        [-1,  0,  1, 2]
//                                        [ 0, -1,  0, 1]
//                                        [ 0,  0, -1, 0]],
// 
// tf.matrix_band_part(input, 2, 1) ==> [[ 0,  1,  0, 0]
//                                       [-1,  0,  1, 0]
//                                       [-2, -1,  0, 1]
//                                       [ 0, -2, -1, 0]]
// ```
// 
// Useful special cases:
// 
// ```prettyprint
//  tf.matrix_band_part(input, 0, -1) ==> Upper triangular part.
//  tf.matrix_band_part(input, -1, 0) ==> Lower triangular part.
//  tf.matrix_band_part(input, 0, 0) ==> Diagonal.
// ```
//
// Arguments:
//	input: Rank `k` tensor.
//	num_lower: 0-D tensor. Number of subdiagonals to keep. If negative, keep entire
// lower triangle.
//	num_upper: 0-D tensor. Number of superdiagonals to keep. If negative, keep
// entire upper triangle.
//
// Returns Rank `k` tensor of the same shape as input. The extracted banded tensor.
func MatrixBandPart(scope *Scope, input tf.Output, num_lower tf.Output, num_upper tf.Output)(band tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixBandPart",
		Input: []tf.Input{
			input, num_lower, num_upper, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// MatrixTriangularSolveAttr is an optional argument to MatrixTriangularSolve.
type MatrixTriangularSolveAttr func(optionalAttr)


// MatrixTriangularSolveLower sets the optional lower attribute to value.
//
// value: Boolean indicating whether the innermost matrices in `matrix` are
// lower or upper triangular.
// If not specified, defaults to true 
func MatrixTriangularSolveLower(value bool) MatrixTriangularSolveAttr {
	return func(m optionalAttr) {
		m["lower"] = value
	}
}

// MatrixTriangularSolveAdjoint sets the optional adjoint attribute to value.
//
// value: Boolean indicating whether to solve with `matrix` or its (block-wise)
//          adjoint.
// 
// @compatibility(numpy)
// Equivalent to np.linalg.triangular_solve
// @end_compatibility
// If not specified, defaults to false 
func MatrixTriangularSolveAdjoint(value bool) MatrixTriangularSolveAttr {
	return func(m optionalAttr) {
		m["adjoint"] = value
	}
}

// Solves systems of linear equations with upper or lower triangular matrices by
//
// backsubstitution.
// 
// `matrix` is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions form
// square matrices. If `lower` is `True` then the strictly upper triangular part
// of each inner-most matrix is assumed to be zero and not accessed.
// If `lower` is False then the strictly lower triangular part of each inner-most
// matrix is assumed to be zero and not accessed.
// `rhs` is a tensor of shape `[..., M, K]`.
// 
// The output is a tensor of shape `[..., M, K]`. If `adjoint` is
// `True` then the innermost matrices in output` satisfy matrix equations
// `matrix[..., :, :] * output[..., :, :] = rhs[..., :, :]`.
// If `adjoint` is `False` then the strictly then the  innermost matrices in
// `output` satisfy matrix equations
// `adjoint(matrix[..., i, k]) * output[..., k, j] = rhs[..., i, j]`.
//
// Arguments:
//	matrix: Shape is `[..., M, M]`.
//	rhs: Shape is `[..., M, K]`.
//
// Returns Shape is `[..., M, K]`.
func MatrixTriangularSolve(scope *Scope, matrix tf.Output, rhs tf.Output, optional ...MatrixTriangularSolveAttr)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	attrs := map[string]interface{}{}
	for _, a := range optional {
		a(attrs)
	}
	opspec := tf.OpSpec{
		Type: "MatrixTriangularSolve",
		Input: []tf.Input{
			matrix, rhs, 
		},
		Attrs: attrs,
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns the batched diagonal part of a batched tensor.
//
// This operation returns a tensor with the `diagonal` part
// of the batched `input`. The `diagonal` part is computed as follows:
// 
// Assume `input` has `k` dimensions `[I, J, K, ..., M, N]`, then the output is a
// tensor of rank `k - 1` with dimensions `[I, J, K, ..., min(M, N)]` where:
// 
// `diagonal[i, j, k, ..., n] = input[i, j, k, ..., n, n]`.
// 
// The input must be at least a matrix.
// 
// For example:
// 
// ```prettyprint
// # 'input' is [[[1, 0, 0, 0]
//                [0, 2, 0, 0]
//                [0, 0, 3, 0]
//                [0, 0, 0, 4]],
//               [[5, 0, 0, 0]
//                [0, 6, 0, 0]
//                [0, 0, 7, 0]
//                [0, 0, 0, 8]]]
// 
// and input.shape = (2, 4, 4)
// 
// tf.matrix_diag_part(input) ==> [[1, 2, 3, 4], [5, 6, 7, 8]]
// 
// which has shape (2, 4)
// ```
//
// Arguments:
//	input: Rank `k` tensor where `k >= 2`.
//
// Returns The extracted diagonal(s) having shape
// `diagonal.shape = input.shape[:-2] + [min(input.shape[-2:])]`.
func MatrixDiagPart(scope *Scope, input tf.Output)(diagonal tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDiagPart",
		Input: []tf.Input{
			input, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Returns a batched diagonal tensor with a given batched diagonal values.
//
// Given a `diagonal`, this operation returns a tensor with the `diagonal` and
// everything else padded with zeros. The diagonal is computed as follows:
// 
// Assume `diagonal` has `k` dimensions `[I, J, K, ..., N]`, then the output is a
// tensor of rank `k+1` with dimensions [I, J, K, ..., N, N]` where:
// 
// `output[i, j, k, ..., m, n] = 1{m=n} * diagonal[i, j, k, ..., n]`.
// 
// For example:
// 
// ```prettyprint
// # 'diagonal' is [[1, 2, 3, 4], [5, 6, 7, 8]]
// 
// and diagonal.shape = (2, 4)
// 
// tf.matrix_diag(diagonal) ==> [[[1, 0, 0, 0]
//                                      [0, 2, 0, 0]
//                                      [0, 0, 3, 0]
//                                      [0, 0, 0, 4]],
//                                     [[5, 0, 0, 0]
//                                      [0, 6, 0, 0]
//                                      [0, 0, 7, 0]
//                                      [0, 0, 0, 8]]]
// 
// which has shape (2, 4, 4)
// ```
//
// Arguments:
//	diagonal: Rank `k`, where `k >= 1`.
//
// Returns Rank `k+1`, with `output.shape = diagonal.shape + [diagonal.shape[-1]]`.
func MatrixDiag(scope *Scope, diagonal tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDiag",
		Input: []tf.Input{
			diagonal, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}

// Computes the determinant of one ore more square matrices.
//
// The input is a tensor of shape `[..., M, M]` whose inner-most 2 dimensions
// form square matrices. The output is a tensor containing the determinants
// for all input submatrices `[..., :, :]`.
//
// Arguments:
//	input: Shape is `[..., M, M]`.
//
// Returns Shape is `[...]`.
func MatrixDeterminant(scope *Scope, input tf.Output)(output tf.Output) {
	if scope.Err() != nil {
		return
	}
	opspec := tf.OpSpec{
		Type: "MatrixDeterminant",
		Input: []tf.Input{
			input, 
		},
	}
	op := scope.AddOperation(opspec)
	return op.Output(0)
}
